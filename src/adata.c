#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "adata.h"

ADATA_T	*
AD_new_adata(const char *fname, int size)
{
	ADATA_T	*adp = NULL;
	int	err = 0;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	if(size <= 0){
		LOG_ERROR("bad size %d, must be > 0", size);
		err = 1;
		goto CLEAN_UP;
	}

	adp = (ADATA_T *)calloc((size_t)1, sizeof(ADATA_T));
	if(adp == NULL){
		LOG_ERROR("can't allocate adp");
		err = 1;
		goto CLEAN_UP;
	}

	adp->a_fname = strdup(fname);
	if(adp->a_fname == NULL){
		LOG_ERROR("can't strdup fname");
		err = 1;
		goto CLEAN_UP;
	}

	adp->as_atab = size;
	adp->a_atab = (ADDR_T **)calloc((size_t)adp->as_atab, sizeof(ADDR_T *));
	if(adp->a_atab == NULL){
		LOG_ERROR("can't alloc atab");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : 

	if(err){
		if(adp != NULL){
			AD_delete_adata(adp);
			adp = NULL;
		}
	}

	return adp;
}

void
AD_delete_adata(ADATA_T *adp)
{

	if(adp == NULL)
		return;

	if(adp->a_atab != NULL){
		int	i;

		for(i = 0; i < adp->as_atab; i++){
			if(adp->a_atab[i] != NULL)
				AD_delete_addr(adp->a_atab[i]);
		}
		free(adp->a_atab);
	}
	if(adp->a_fp != NULL)
		fclose(adp->a_fp);

	if(adp->a_fname != NULL)
		free(adp->a_fname);

	free(adp);
}

int
AD_read_adata(ADATA_T *adp, int verbose)
{
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lnum, fnum;
	char	*s_fp, *e_fp;
	double	lng, lat;
	ADDR_T	*ap = NULL;
	int	err = 0;

	if(adp == NULL){
		LOG_ERROR("adp is NULL");
		err = 1;
		goto CLEAN_UP;
	}

	if(adp->a_fname == NULL || *adp->a_fname == '\0'){
		LOG_ERROR("a_fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	if((adp->a_fp = fopen(adp->a_fname, "r")) == NULL){
		LOG_ERROR("can't read adata file %s", adp->a_fname);
		err = 1;
		goto CLEAN_UP;
	}

	// file format is 6 fields, tab separated
	//	text	text	text	lng	lat	text
	for(lnum = 0; (l_line = getline(&line, &s_line, adp->a_fp)) > 0; ){
		lnum++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_ERROR("line %7d: empty line", lnum);
				err = 1;
				goto CLEAN_UP;
			}
		}
		for(fnum = 0, e_fp = s_fp = line; *s_fp; ){
			fnum++;
			if((e_fp = strchr(s_fp, '\t')) == NULL)
				e_fp = s_fp + strlen(s_fp);
			if(fnum == 4)
				lng = strtod(s_fp, NULL);
			else if(fnum == 5)
				lat = strtod(s_fp, NULL);
			s_fp = *e_fp ? e_fp + 1 : e_fp;
		}
		if(fnum != 6){
			LOG_ERROR("line %7d: wrong number of fields %d, need %d", lnum, fnum, 6);
			err = 1;
			goto CLEAN_UP;
		}
		ap = AD_new_addr(line, lnum, 1);
		if(ap == NULL){
			LOG_ERROR("line %7d: AD_new_addr failed", lnum);
			err = 1;
			goto CLEAN_UP;
		}
		if(adp->an_atab >= adp->as_atab){
			void	*s_tab;
			int	s_size;

			s_tab = adp->a_atab;
			s_size = adp->as_atab;
			
			adp->as_atab *= 1.5;
			adp->a_atab = realloc(adp->a_atab, adp->as_atab);
			if(adp->a_atab == NULL){
				LOG_ERROR("line %7d: can't realloc a_atab", lnum);
				err = 1;
				adp->as_atab = s_size;
				adp->a_atab = s_tab;
				goto CLEAN_UP;
			}
		}
		adp->a_atab[adp->an_atab] = ap;
		adp->an_atab++;
		ap = NULL;
	}
	if(adp->an_atab == 0){
		LOG_ERROR("no addresses");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(line != NULL)
		free(line);

	if(adp->a_fp != NULL){
		fclose(adp->a_fp);
		adp->a_fp = NULL;
	}

	return err;
}

void
AD_dump_adata(FILE *fp, const ADATA_T *adp, int verbose)
{

	if(adp == NULL){
		fprintf(fp, "adp = NULL\n");
		return;
	}

	fprintf(fp, "adata = {\n");
	fprintf(fp, "\tfname  = %s\n", adp->a_fname ? adp->a_fname : "NULL");
	fprintf(fp, "\tfp     = %p\n", adp->a_fp);
	fprintf(fp, "\ts_atab = %d\n", adp->as_atab);
	fprintf(fp, "\tn_atab = %d\n", adp->an_atab);
	if(verbose > 0){
		int	i;
		const ADDR_T	*ap;

		fprintf(fp, "\tlines = {\n");
		for(i = 0; i < adp->an_atab; i++){
			ap = adp->a_atab[i];
			if(verbose == 1)
				fprintf(fp, "\t\t%d\t%d\t%.15e\t%15e\t%s\n", ap->a_sign, ap->a_lnum, ap->a_lng, ap->a_lat, ap->a_line);
			else{
				fprintf(fp, "\t\tline = %d {\n", i + 1);
				fprintf(fp, "\t\t\tsing = %d\n", ap->a_sign);
				fprintf(fp, "\t\t\tlnum = %d\n", ap->a_lnum);
				fprintf(fp, "\t\t\tqry  = %s\n", ap->a_qry ? ap->a_qry : "NULL");
				fprintf(fp, "\t\t\tlng  = %.15e\n", ap->a_lng);
				fprintf(fp, "\t\t\tlat  = %.15e\n", ap->a_lat);
				fprintf(fp, "\t\t\trply = %s\n", ap->a_rply);
				fprintf(fp, "\t\t}\n");
			}
		}
		fprintf(fp, "\t}\n");
	}
	fprintf(fp, "\n");
}

ADDR_T	*
AD_new_addr(const char *line, int lnum, int sign)
{
	const char	*s_fp, *e_fp;
	char	*qry = NULL;
	double	lng, lat;
	char	*rply = NULL;
	int	fnum;
	ADDR_T	*ap = NULL;
	int	err = 0;

	if(line == NULL || *line == '\0'){
		LOG_ERROR("line is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	for(fnum = 0, e_fp = s_fp = line; *s_fp; ){
		fnum++;
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);
		if(fnum == 2){
			qry = strndup(s_fp, e_fp - s_fp);
			if(qry == NULL){
				LOG_ERROR("line %7d: can't strndup qry", lnum);
				err = 1;
				goto CLEAN_UP;
			}
		}else if(fnum == 4)
			lng = strtod(s_fp, NULL);
		else if(fnum == 5)
			lat = strtod(s_fp, NULL);
		else if(fnum == 6){
			rply = strndup(s_fp, e_fp - s_fp);
			if(rply == NULL){
				LOG_ERROR("line %7d: can't strndup rply", lnum);
				err = 1;
				goto CLEAN_UP;
			}
		}
		s_fp = *e_fp ? e_fp + 1 : e_fp;
	}
	if(fnum != 6){
		LOG_ERROR("line %7d: wrong number of fields %d, need %d", lnum, fnum, 6);
		err = 1;
		goto CLEAN_UP;
	}

	ap = (ADDR_T *)calloc((size_t)1, sizeof(ADDR_T));
	if(ap == NULL){
		LOG_ERROR("can't allocate ap");
		err = 1;
		goto CLEAN_UP;
	}

	ap->a_sign = sign;
	ap->a_lnum = lnum;
	ap->a_line = strdup(line);
	if(ap->a_line == NULL){
		LOG_ERROR("can't strdup line");
		err = 1;
		goto CLEAN_UP;
	}
	ap->a_qry = qry;
	qry = NULL;
	ap->a_lng = lng;
	ap->a_lat = lat;
	ap->a_rply = rply;
	rply = NULL;

CLEAN_UP : ;

	if(rply != NULL)
		free(rply);
	if(qry != NULL)
		free(qry);

	if(err){
		if(ap != NULL){
			AD_delete_addr(ap);
			ap = NULL;
		}
	}

	return ap;
}

void
AD_delete_addr(ADDR_T *ap)
{

	if(ap == NULL)
		return;

	if(ap->a_line != NULL)
		free(ap->a_line);
	if(ap->a_qry != NULL)
		free(ap->a_qry);
	if(ap->a_rply != NULL)
		free(ap->a_rply);

	free(ap);
}

void
AD_print_addr(FILE *fp, const ADDR_T *ap)
{
	const char *q1, *e_q1;
	const char *r1, *e_r1;
	int	f;
	char	*s_fp, *e_fp;;

	if(ap->a_sign){
		fprintf(fp, "%s\n", ap->a_line);
		return;
	}

	for(q1 = ap->a_qry; isspace(*q1); q1++)
		;
	e_q1 = strchr(q1, ',');
	for(--e_q1; isspace(*e_q1); e_q1--)
		;

	for(r1 = ap->a_rply; isspace(*r1); r1++)
		;
	e_r1 = strchr(r1, ',');
	for(--e_r1; isspace(*e_r1); e_r1--)
		;

	// qry & rply are the same, nothing to do
	if(!strncmp(q1, r1, e_q1 - q1)){
		fprintf(fp, "%s\n", ap->a_line);
		return;
	}

	for(f = 0, s_fp = ap->a_line; *s_fp; ){
		f++;
		if(f == 6)
			break;
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);
		s_fp = *e_fp ? e_fp + 1 : e_fp;
	}

	// LOG_DEBUG("%.*s\t%.*s, %s", (int)(s_fp - ap->a_line), ap->a_line, (int)(e_q1 - q1 + 1), q1, s_fp);

	fprintf(fp, "%.*s%.*s, %s\n", (int)(s_fp - ap->a_line), ap->a_line, (int)(e_q1 - q1 + 1), q1, s_fp);
}
