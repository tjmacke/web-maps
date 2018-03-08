#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"
#include "dbase.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0", "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0", "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-trim", 1, AVK_NONE, AVT_BOOL, "0", "Use -term to remove trailing spaces when dumping records."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	int	trim = 0;
	int	ftype = SFT_UNKNOWN;
	FILE	*fp = NULL;
	SF_FHDR_T	*fhdr = NULL;
	DBF_META_T	*dbm = NULL;
	int	i, c, h_pkey_cands;
	char	*rbuf = NULL;
	size_t	s_rbuf = 0;
	ssize_t	l_rbuf;
	
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 1, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-trim", AVT_BOOL);
	trim = a_val->av_value.v_int;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	ftype = SHP_get_file_type(args->a_files[0]);
	if(ftype == SFT_UNKNOWN){
		LOG_ERROR("SHP_get_file_type failed for %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read input file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	switch(ftype){
	case SFT_SHP :
	case SFT_SHX :
		fhdr = SHP_open_file(args->a_files[0]);
		if(fhdr == NULL){
			LOG_ERROR("SHP_open_file failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
		SHP_dump_fhdr(stdout, fhdr);
		if(verbose == 0)
			break;
		if(ftype == SFT_SHP){
			int	c, n_recs;
			SF_SHAPE_T	*shp = NULL;

			// dump shp file w/o use of shx index
			for(n_recs = 0; (c = getc(fhdr->s_fp)) != EOF; ){
				ungetc(c, fhdr->s_fp);
				shp = SHP_read_shape(fhdr->s_fp);
				if(shp == NULL){
					LOG_ERROR("SHP_read_shape failed for record %d", n_recs+1);
					err = 1;
					goto CLEAN_UP;
				}
				n_recs++;
				SHP_dump_shape(stdout, shp, verbose);
				SHP_delete_shape(shp);
				shp = NULL;
			}
			LOG_INFO("%d recs", n_recs);
		}else{
			int	i, n_recs;
			SF_RIDX_T	ridx;

			n_recs = (fhdr->sl_file - SF_FHDR_SIZE) / SF_RIDX_SIZE;
			for(i = 0; i < n_recs; i++){
				if(SHP_read_ridx(fhdr->s_fp, &ridx)){
					LOG_ERROR("SHP_read_ridx failed for record %d", i+1);
					err = 1;
					goto CLEAN_UP;
				}
				SHP_dump_ridx(stdout, &ridx);
			}
		}
		break;
	case SFT_DBF :
		// pkey candidates require reading all the recs as the last rec may invalidate a candidate
		// So, DBF_read_meta() sets d_is_pkey_cand to 0 if it's not an int and -1 (not known)  if it is.
		dbm = DBF_new_dbf_meta(args->a_files[0]);
		if(dbm == NULL){
			LOG_ERROR("DBF_new_dbf_meta failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
		if(DBF_read_dbf_meta(fp, dbm)){
			LOG_ERROR("DBF_read_dbf_meta failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
		DBF_dump_dbf_meta(stdout, dbm, verbose);
		s_rbuf = dbm->d_fhdr->dl_rec + 1;
		rbuf = (char *)malloc(s_rbuf * sizeof(char));
		if(rbuf == NULL){
			LOG_ERROR("can't allocate rbuf");
			err = 1;
			goto CLEAN_UP;
		}
		// check for possibley primary key fields.  They must have type integer
		// and the value must equal the record number.
		// Step 1, assume fields not previously disqualified are primary keys.
		// Step 2. read the records and disqualify any where the value != rec num
		// Step 3. Any survivors are primary key candidates
		for(i = 0; i < dbm->dn_fields; i++){
			if(dbm->d_fields[i]->d_is_pkey_cand == -1)
				dbm->d_fields[i]->d_is_pkey_cand = 1;
		}
		for(i = 0; i < dbm->d_fhdr->dn_recs; i++){
			l_rbuf = fread(rbuf, sizeof(char), dbm->d_fhdr->dl_rec, fp);
			if(l_rbuf != dbm->d_fhdr->dl_rec){
				LOG_ERROR("%s:%7d: bad read: got %ld, need %d", args->a_files[0], i+1, l_rbuf, dbm->d_fhdr->dl_rec);
				err = 1;
				goto CLEAN_UP;
			}
			rbuf[l_rbuf] = '\0';	// records are ascii
			DBF_chk_pkey_cands(dbm, i+1, rbuf);
			if(verbose)
				DBF_dump_rec(stdout, dbm, verbose, trim, i+1, rbuf);
		}
		c = getc(fp);
		if(c == EOF){
			LOG_WARN("%s: missing eof char", args->a_files[0]);
			err = 1;
		}else if(c != DBF_EOF){
			LOG_ERROR("%s: bad eof char: 0x%02x, need 0x%02x", args->a_files[0], c & 0xff, DBF_EOF);
			err = 1;
			goto CLEAN_UP;
		}
		for(h_pkey_cands = i = 0; i < dbm->dn_fields; i++){
			if(dbm->d_fields[i]->d_is_pkey_cand == 1){
				if(!h_pkey_cands){
					h_pkey_cands = 1;
					printf("pkey_cands = {\n");
				}
				printf("\t%s\n", dbm->d_fields[i]->d_name); 
			}
		}
		printf("%s\n", h_pkey_cands ? "}" : "pkey_cands = None");
		break;
	case SFT_PRJ :
		break;
	default :
		LOG_ERROR("unkonwn file type %d", ftype);
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(rbuf != NULL)
		free(rbuf);

	if(fp != NULL)
		fclose(fp);

	if(fhdr != NULL)
		SHP_close_file(fhdr);

	if(dbm != NULL)
		DBF_delete_dbf_meta(dbm);

	TJM_free_args(args);

	exit(err);
}
