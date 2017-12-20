#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "dbase.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-t",    0, AVK_REQ,  AVT_STR,  NULL, "Use -t T to put the dbase data into a sqlite table named T."},
	{"-pk",   0, AVK_REQ,  AVT_STR,  NULL, "Use -pk F to use field F as the primary key; use -pk +F to add field F as the primary key."},
	{"-kth",  1, AVK_REQ,  AVT_STR,  NULL, "Use -kth D to look for known tables in directory D, instead of $WM_HOME/etc."},
	{"-ktl",  1, AVK_REQ,  AVT_STR,  NULL, "Use -ktl L, L comma separated list of known tables to add to the new db."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

static	int
add_known_tables(const char *, const char *);

static	int
mk_known_table(FILE *, const char *);

static	int
mk_create_table_cmd(const DBF_META_T *, const char *, const char *);

static	int
mk_insert_cmd(const DBF_META_T *, const char *, const char *, const char *);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*tname = NULL;
	const char	*pk = NULL;
	const char	*kt_home = NULL;
	const char	*kt_list = NULL;
	FILE	*fp = NULL;
	DBF_META_T	*dbm = NULL;
	char	*rbuf = NULL;
	size_t	s_rbuf = 0;
	ssize_t	n_rbuf;
	int	i;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-t", AVT_STR);
	tname = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pk", AVT_STR);
	pk = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-kth", AVT_STR);
	kt_home = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-ktl", AVT_STR);
	kt_list = a_val->av_value.v_str;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	if(kt_list != NULL){
		if(add_known_tables(kt_home, kt_list)){
			LOG_ERROR("add_known_tables failed");
			err = 1;
			goto CLEAN_UP;
		}
	}

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read dbase file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	dbm = DBF_new_dbf_meta(args->an_files == 0 ? "--stdin--" : args->a_files[0]);
	if(dbm == NULL){
		LOG_ERROR("DBF_new_dbf_meta failed for %s", args->an_files == 0 ? "--stdin--" : args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}
	if(DBF_read_dbf_meta(fp, dbm)){
		LOG_ERROR("DBF_read_dbf_meta failed for %s", args->an_files == 0 ? "--stdin--" : args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	if(mk_create_table_cmd(dbm, tname, pk)){
		LOG_ERROR("mk_create_table_cmd failed");
		err = 1;
		goto CLEAN_UP;
	}

	s_rbuf = dbm->d_fhdr->dl_rec + 1;
	rbuf = (char *)malloc(s_rbuf * sizeof(char));
	if(rbuf == NULL){
		LOG_ERROR("can't allocate rbuf");
		err = 1;
		goto CLEAN_UP;
	}
	for(i = 0; i < dbm->d_fhdr->dn_recs; i++){
		n_rbuf = fread(rbuf, sizeof(char), dbm->d_fhdr->dl_rec, fp);
		if(n_rbuf != dbm->d_fhdr->dl_rec){
			LOG_ERROR("rec %7d: bad read: expect %d bytes, got %ld", i+1, dbm->d_fhdr->dl_rec, n_rbuf);
			err = 1;
			goto CLEAN_UP;
		}
		rbuf[n_rbuf] = '\0';	// data is ASCII!
		if(mk_insert_cmd(dbm, tname, pk, rbuf)){
			LOG_ERROR("rec %7d: mk_insert_cmd failed", i+1);
			err = 1;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;

	if(rbuf != NULL)
		free(rbuf);

	if(dbm != NULL)
		DBF_delete_dbf_meta(dbm);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}

static	int
add_known_tables(const char *kt_home, const char *kt_list)
{
	const char	*wm_home = NULL;
	char	*fname = NULL;
	size_t	s_fname = 0;
	const char	*s_kt, *e_kt;
	char	*tname = NULL;
	char	*dot;
	FILE	*fp = NULL;
	int	err = 0;

	if(kt_list == NULL || *kt_list == '\0'){
		LOG_ERROR("kt_list is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	if(kt_home == NULL || *kt_home == '\0'){
		if((wm_home = getenv("WM_HOME")) == NULL){
			LOG_ERROR("environment variable WM_HOME is not set, kt_home must be set");
			err = 1;
			goto CLEAN_UP;
		}

LOG_DEBUG("wm_home = %s", wm_home);

	}

LOG_DEBUG("kt_list = %s", kt_list);

	s_fname = 0;
	for(s_kt = e_kt = kt_list; *s_kt; ){
		if((e_kt = strchr(s_kt, ',')) == NULL){
			e_kt = s_kt + strlen(s_kt);
		}else if(e_kt[1] == '\0'){
			LOG_ERROR("empty last table name: %s", kt_list);
			err = 1;
			goto CLEAN_UP;
		}
		if(s_kt == e_kt){
			LOG_ERROR("empty table name: %s", kt_list);
			err = 1;
			goto CLEAN_UP;
		}
		if(e_kt - s_kt > s_fname)
			s_fname = e_kt - s_kt;
		s_kt = *e_kt ? e_kt + 1 : e_kt;
	}

LOG_DEBUG("s_fname = %ld", s_fname);

	s_fname = (wm_home == NULL ? strlen(kt_home) + 1 : strlen("/etc/")) + 1;
	fname = (char *)malloc(s_fname * sizeof(char));
	if(fname == NULL){
		LOG_ERROR("can't allocate fname");
		err = 1;
		goto CLEAN_UP;
	}

	for(s_kt = e_kt = kt_list; *s_kt; ){
		if((e_kt = strchr(s_kt, ',')) == NULL)
			e_kt = s_kt + strlen(s_kt);
	
		tname = strndup(s_kt, e_kt - s_kt);
		if(tname == NULL){
			LOG_ERROR("can't strndup tname: %.*s", (int)(e_kt - s_kt), s_kt);
			err = 1;
			goto CLEAN_UP;
		}
		if((dot = strchr(tname, '.')) != NULL)
			*dot = '\0';

		if(wm_home != NULL)
			sprintf(fname, "%s/etc/%.*s", wm_home, (int)(e_kt - s_kt), s_kt);
		else
			sprintf(fname, "%s/%.*s", kt_home, (int)(e_kt - s_kt), s_kt);

LOG_DEBUG("make known table %s from file %s", tname, fname);

		if((fp = fopen(fname, "r")) == NULL){
			LOG_ERROR("can't read known table file %s", fname);
			err = 1;
			goto CLEAN_UP;
		}
		if(mk_known_table(fp, tname)){
			LOG_ERROR("mk_known_table faile for %s", tname);
			err = 1;
			goto CLEAN_UP;
		}
		fclose(fp);
		fp = NULL;
		free(tname);
		tname = NULL;
		s_kt = *e_kt ? e_kt + 1 : e_kt;
	}

CLEAN_UP : ;

	if(fp != NULL)
		fclose(fp);

	if(tname != NULL)
		free(tname);

	if(fname != NULL)
		free(fname);

	return err;
}

static	int
mk_known_table(FILE *fp, const char *tname)
{
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lcnt;
	char	**fields = NULL;
	int	n_fields, nf;
	char	*s_fp, *e_fp;
	int	err = 0;

	if((l_line = getline(&line, &s_line, fp)) <= 0){
		LOG_ERROR("table %s: no header", tname);
		err = 1;
		goto CLEAN_UP;
	}
	if(line[l_line - 1] == '\n'){
		line[l_line - 1] = '\0';
		l_line--;
		if(l_line == 0){
			LOG_ERROR("table %s: empty header", tname);
			err = 1;
			goto CLEAN_UP;
		}
	}

	for(n_fields = 0, e_fp = s_fp = line; *s_fp; ){
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);
		n_fields++;
		s_fp = *e_fp ? e_fp + 1 : e_fp;
	}
	fields = (char **)calloc((size_t)n_fields, sizeof(char *));
	if(fields == NULL){
		LOG_ERROR("can't allocate fields");
		err = 1;
		goto CLEAN_UP;
	}

	printf("CREATE TABLE %s (\n", tname);
	for(nf = 0, e_fp = s_fp = line; *s_fp; ){
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);
		fields[nf] = strndup(s_fp, e_fp - s_fp);
		if(fields[nf] == NULL){
			LOG_ERROR("can't strndup fields[%d] = %.*s", nf, (int)(e_fp - s_fp), s_fp);
			err = 1;
			goto CLEAN_UP;
		}
		printf("\t%s text NOT NULL,\n", fields[nf]);
		nf++;
		s_fp = *e_fp ? e_fp + 1 : e_fp;
	}
	printf("\tPRIMARY KEY(%s ASC)\n", fields[0]);
	printf(") ;\n");
	for(lcnt = 1; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_ERROR("table %s: line %d: empty header", tname, lcnt);
				err = 1;
				goto CLEAN_UP;
			}
		}
		for(nf = 0, e_fp = s_fp = line; *s_fp; ){
			if((e_fp = strchr(s_fp, '\t')) == NULL)
				e_fp = s_fp + strlen(s_fp);
			nf++;
			s_fp = *e_fp ? e_fp + 1 : e_fp;
		}
		if(nf != n_fields){
			LOG_ERROR("table %s: line %d: wrong number of fields: %d, need %d", tname, lcnt, nf, n_fields);
			err = 1;
			goto CLEAN_UP;
		}
		printf("INSERT INTO %s (\n", tname);
		for(nf = 0; nf < n_fields; nf++)
			printf("\t%s%s\n", fields[nf], nf < n_fields - 1 ? "," : "");
		printf(") VALUES (\n");
		for(nf = 0, e_fp = s_fp = line; *s_fp; ){
			if((e_fp = strchr(s_fp, '\t')) == NULL)
				e_fp = s_fp + strlen(s_fp);
			printf("\t'");
			for( ; s_fp < e_fp; s_fp++){
				if(*s_fp == '\'')
					putchar('\'');
				putchar(*s_fp);
			}
			printf("'%s\n", nf < n_fields - 1 ? "," : "");
			nf++;
			s_fp = *e_fp ? e_fp + 1 : e_fp;
		}
		printf(") ;\n");
	}

CLEAN_UP : ;

	if(fields != NULL){
		int	f;
		for(f = 0; f < n_fields; f++){
			if(fields[f] != NULL)
				free(fields[f]);
		}
		free(fields);
	}

	if(line != NULL)
		free(line);

	return err;
}

static	int
mk_create_table_cmd(const DBF_META_T *dbm, const char *tname, const char *pk)
{
	int	add_pk = 0;
	int	i;
	DBF_FIELD_T	*fldp;
	int	err = 0;

	// check if we need to add pk
	// else check that pk is a field that can be a key

	if(*pk == '+'){
		add_pk = 1;
		pk++;
	}

	for(i = 0; i < dbm->dn_fields; i++){
		fldp = dbm->d_fields[i];
		if(!strcmp(fldp->d_name, pk)){
			if(fldp->d_type != 'N'){
				LOG_ERROR("requested primary key field %s has wrong type %c", pk, fldp->d_type);
				err = 1;
				goto CLEAN_UP;
			}else if(fldp->d_dec_count != 0){
				LOG_ERROR("requested primary key field %s has non-zero dec_count type %d", pk, fldp->d_type);
				err = 1;
				goto CLEAN_UP;
			}
			if(add_pk){
				LOG_WARN("requested primary key field %s already in db", pk);
				add_pk = 0;
			}
			break;
		}
	}

	printf("CREATE TABLE %s (\n", tname);
	if(add_pk)
		printf("\t%s integer NOT NULL,\n", pk);
	for(i = 0; i < dbm->dn_fields; i++){
		fldp = dbm->d_fields[i];
		printf("\t%s", fldp->d_name);
		if(fldp->d_type == 'C')
			printf(" text");
		else if(fldp->d_type == 'N')
			printf(" %s", fldp->d_dec_count == 0 ? "integer" : "double");
		else{
			LOG_ERROR("unknown field type %c", fldp->d_type);
			err = 1;
			goto CLEAN_UP;
		}
		printf(" NOT NULL,\n");
	}
	printf("\tPRIMARY KEY(%s ASC)\n", pk );
	printf(") ;\n");

CLEAN_UP : ;

	return err;
}

static	int
mk_insert_cmd(const DBF_META_T *dbm, const char *tname, const char * pk, const char *rbuf)
{
	int	add_pk = 0;
	int	i;
	DBF_FIELD_T	*fldp;
	char	*fval = NULL;
	char	*fvp;
	int	err = 0;

	if(*pk == '+'){
		add_pk = 1;
		pk++;
	}

	printf("INSERT INTO %s (\n", tname);
		for(i = 0; i < dbm->dn_fields; i++){
			fldp = dbm->d_fields[i];
			printf("\t%s%s\n", fldp->d_name, i < dbm->dn_fields - 1 ? "," : "");
		}
	printf(") VALUES (\n");
	for(i = 0; i < dbm->dn_fields; i++){
		fldp = dbm->d_fields[i];
		fval = DBF_get_field_value(fldp, rbuf, 1);
		if(fval == NULL){
			LOG_ERROR("DBF_get_field_value failed for field %s", fldp->d_name);
			err = 1;
			goto CLEAN_UP;
		}
		if(fldp->d_type != 'C')
			printf("\t%s", fval);
		else{
			printf("\t'");
			for(fvp = fval; *fvp; fvp++){
				if(*fvp == '\'')
					putchar('\'');
				putchar(*fvp);
			}
			printf("'");
		}
		if(i < dbm->dn_fields - 1)
			printf(",");
		printf("\n");
	}
	printf(") ;\n");

CLEAN_UP : ;

	if(fval != NULL)
		free(fval);

	return err;
}
