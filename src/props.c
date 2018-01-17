#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "props.h"

static	int
props_cmp_key(const void *, const void *);

PROPERTIES_T	*
PROPS_new_properties(const char *fname, const char *pkey)
{
	int	err = 0;
	PROPERTIES_T	*props = NULL;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	if(pkey == NULL || *pkey == '\0'){
		LOG_ERROR("pkey is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	props = (PROPERTIES_T *)calloc((size_t)1, sizeof(PROPERTIES_T));
	if(props == NULL){
		LOG_ERROR("can't allocatte props");
		err = 1;
		goto CLEAN_UP;
	}
	props->p_fname = strdup(fname);
	if(props->p_fname == NULL){
		LOG_ERROR("can't strdup fname %s", fname);
		err = 1;
		goto CLEAN_UP;
	}
	props->p_pkey = strdup(pkey);
	if(props->p_pkey == NULL){
		LOG_ERROR("can't strdup pkey %s", pkey);
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(err){
		PROPS_delete_properties(props);
		props = NULL;
	}

	return props;
}

void
PROPS_delete_properties(PROPERTIES_T	*props)
{
	if(props == NULL)
		return;

	if(props->p_ptab != NULL){
		int	p;

		for(p = 0; p < props->pn_ptab; p++){
			if(props->p_ptab[p] != NULL)
				PROPS_delete_prop(props->p_ptab[p]);
		}
		free(props->p_ptab);
	}

	if(props->p_hdr != NULL)
		free(props->p_hdr);

	if(props->p_pkey != NULL)
		free(props->p_pkey);

	if(props->p_fname != NULL)
		free(props->p_fname);

	free(props);
}

PROP_T	*
PROPS_new_prop(const char *key, const char *value)
{
	PROP_T	*pp = NULL;
	int	err = 0;

	if(key == NULL || *key == '\0'){
		LOG_ERROR("key is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	if(value == NULL || *value == '\0'){
		LOG_ERROR("value s NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	pp = (PROP_T *)malloc(sizeof(PROP_T));
	if(pp == NULL){
		LOG_ERROR("can't allocate pp");
		err = 1;
		goto CLEAN_UP;
	}
	pp->p_key = atoi(key);
	pp->p_value = strdup(value);
	if(pp->p_value == NULL){
		LOG_ERROR("can't allocate p_value");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(err){
		PROPS_delete_prop(pp);
		pp = NULL;
	}

	return pp;
}

void
PROPS_delete_prop(PROP_T *pp)
{

	if(pp != NULL){
		if(pp->p_value != NULL)
			free(pp->p_value);
		free(pp);
	}
}

int
PROPS_read_properties(PROPERTIES_T *props)
{
	FILE	*fp = NULL;
	char	*line = NULL;
	size_t	s_line;
	ssize_t	l_line;
	int	lcnt;
	int	nf;
	char	*s_fp, *e_fp;
	char	*pkey = NULL;
	PROP_T	*pp;
	int	err = 0;

	if((fp = fopen(props->p_fname, "r")) == NULL){
		LOG_ERROR("can't read property file %s", props->p_fname);
		err = 1;
		goto CLEAN_UP;
	}

	for(lcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; )
		lcnt++;
	if(lcnt == 0){
		LOG_ERROR("property file is empty");
		err = 1;
		goto CLEAN_UP;
	}else if(lcnt == 1){
		LOG_ERROR("property file contains only the header");
		err = 1;
		goto CLEAN_UP;
	}

	props->pn_ptab = (lcnt - 1);	// first line is the header
	props->p_ptab = (PROP_T **)calloc(props->pn_ptab, sizeof(PROP_T *));
	if(props->p_ptab == NULL){
		LOG_ERROR("can't allocate ptab");
		err = 1;
		goto CLEAN_UP;
	}

	rewind(fp);
	for(lcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_ERROR("line %7d: empty line", lcnt);
				err = 1;
				goto CLEAN_UP;
			}
		}
		if(lcnt == 1){
			props->p_hdr = strdup(line);
			if(props->p_hdr == NULL){
				LOG_ERROR("can't strdup header line");
				err = 1;
				goto CLEAN_UP;
			}
			// get the number of fields, set pf_pkey, pf_title
			for(nf = 0, e_fp = s_fp = line; *s_fp; ){
				nf++;
				if((e_fp = strchr(s_fp, '\t')) == NULL)
					e_fp = s_fp + strlen(s_fp);
				if(!strncmp(props->p_pkey, s_fp, e_fp - s_fp))
					props->pf_pkey = nf;
				else if(!strncmp("title", s_fp, e_fp - s_fp))
					props->pf_title = nf;
				s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
			}
			if(props->pf_pkey == 0){
				LOG_ERROR("pkey field %s not in header", props->p_pkey);
				err = 1;
				goto CLEAN_UP;
			}
			if(props->pf_title == 0){
				LOG_ERROR("title field %s not in header", "title");
				err = 1;
				goto CLEAN_UP;
			}
			props->pn_fields = nf;
		}else{
			for(nf = 0, e_fp = s_fp = line; *s_fp; ){
				nf++;
				if((e_fp = strchr(s_fp, '\t')) == NULL)
					e_fp = s_fp + strlen(s_fp);
				if(nf == props->pf_pkey){
					if(e_fp == s_fp){
						LOG_ERROR("line %7d: pkey field is empty", lcnt);
						err = 1;
						goto CLEAN_UP;
					}
					pkey = strndup(s_fp, e_fp - s_fp);
					if(pkey == NULL){
						LOG_ERROR("line %7d: can't strndup pkey field", lcnt);
						err = 1;
						goto CLEAN_UP;
					}
				}else if(nf == props->pf_title){
					if(e_fp == s_fp){
						LOG_ERROR("line %7d: title field is empty", lcnt);
						err = 1;
						goto CLEAN_UP;
					}
				}
				s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
			}
			if(nf != props->pn_fields){
				LOG_ERROR("line %d: wrong number of fields %d, must be %d", lcnt, nf, props->pn_fields);
				err = 1;
				goto CLEAN_UP;
			}
			pp = PROPS_new_prop(pkey, line);
			if(pp == NULL){
				LOG_ERROR("line %d: PROPS_new_prop failed", lcnt);
				err = 1;
				goto CLEAN_UP;
			}
			props->p_ptab[lcnt - 2] = pp;
			free(pkey);
			pkey = NULL;
		}
	}
	qsort(props->p_ptab, props->pn_ptab, sizeof(PROP_T *), props_cmp_key);

CLEAN_UP : ;

	if(pkey != NULL)
		free(pkey);

	if(line != NULL)
		free(line);

	if(fp != NULL)
		fclose(fp);

	if(err){
		PROPS_delete_properties(props);
		props = NULL;
	}

	return err;
}

const PROP_T	*
PROPS_find_props_for_record(PROPERTIES_T *props, int pkey)
{
	int	i, j, k;
	PROP_T	*pp;

	for(i = 0, j = props->pn_ptab - 1; i <= j; ){
		k = (i + j) / 2;
		pp = props->p_ptab[k];
		if(pp->p_key == pkey)
			return pp;
		else if(pp->p_key < pkey)
			i = k + 1;
		else
			j = k - 1;
	}
	return NULL;
}

void
PROPS_dump_properties(FILE *fp, PROPERTIES_T *props)
{
	int	p;
	PROP_T	*pp;

	if(props == NULL){
		fprintf(fp, "props = NULL\n");
		return;
	}

	fprintf(fp, "props = %d {\n", props->pn_ptab);
	for(p = 0; p < props->pn_ptab; p++){
		pp = props->p_ptab[p];
		if(pp == NULL)
			fprintf(fp, "\t%d = NULL\n", p+1);
		else{
			fprintf(fp, "\t%d = {\n", p+1);
			fprintf(fp, "\t\tkey   = %d\n", pp->p_key);
			fprintf(fp, "\t\tvalue = %s\n", pp->p_value ? pp->p_value : "NULL");
			fprintf(fp, "\t}\n");
		}
	}
	fprintf(fp, "}\n");
}

char	*
PROPS_to_json_object(const PROPERTIES_T *props, const PROP_T *pp)
{
	char	**keys = NULL;
	char	**values = NULL;
	int	i;
	char	*s_fp, *e_fp;
	char	*sp = NULL;
	char	*obj = NULL;
	int	err = 0;

	if(props == NULL){
		LOG_ERROR("props is NULL");
		err = 1;
		goto CLEAN_UP;
	}

	if(pp == NULL){
		LOG_ERROR("pp is NULL");
		err = 1;
		goto CLEAN_UP;
	}

	if(pp->p_value == NULL || *pp->p_value == '\0'){
		LOG_ERROR("pp->value is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	keys = (char **)calloc((size_t)props->pn_fields, sizeof(char *));
	if(keys == NULL){
		LOG_ERROR("can't allocate keys");
		err = 1;
		goto CLEAN_UP;
	}

	values = (char **)calloc((size_t)props->pn_fields, sizeof(char *));
	if(values == NULL){
		LOG_ERROR("can't allocate values");
		err = 1;
		goto CLEAN_UP;
	}

	// get the keys from the header
	for(i = 0, e_fp = s_fp = props->p_hdr; *s_fp; ){
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);
		sp = e_fp == s_fp ? strdup("") : strndup(s_fp, e_fp - s_fp);
		if(sp == NULL){
			LOG_ERROR("can't strdup header field %d, \"%.*s\"", i+1, (int)(e_fp - s_fp), s_fp);
			err = 1;
			goto CLEAN_UP;
		}
		keys[i] = sp;
		sp = NULL;
		s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
		i++;
	}

	// get the values from pp->p_value
	for(i = 0, e_fp = s_fp = pp->p_value; *s_fp; ){
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);
		sp = e_fp == s_fp ? strdup("") : strndup(s_fp, e_fp - s_fp);
		if(sp == NULL){
			LOG_ERROR("can't strdup value field %d, \"%.*s\"", i+1, (int)(e_fp - s_fp), s_fp);
			err = 1;
			goto CLEAN_UP;
		}
		values[i] = sp;
		sp = NULL;
		s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
		i++;
	}

	for(i = 0; i < props->pn_fields; i++)
		fprintf(stderr, "%s -> %s\n", keys[i], values[i]);

CLEAN_UP : ;

	if(sp != NULL)
		free(sp);

	if(values != NULL){
		for(i = 0; i < props->pn_fields; i++){
			if(values[i] != NULL)
				free(values[i]);
		}
		free(values);
	}

	if(keys != NULL){
		for(i = 0; i < props->pn_fields; i++){
			if(keys[i] != NULL)
				free(keys[i]);
		}
		free(keys);
	}

	if(err){
		if(obj != NULL)
			free(obj);
		obj = NULL;
	}

	return obj;
}

static	int
props_cmp_key(const void *p1, const void *p2)
{
	const PROP_T	**pp1 = (const PROP_T **)p1;
	const PROP_T	**pp2 = (const PROP_T **)p2;

	if((*pp1)->p_key < (*pp2)->p_key)
		return -1;
	else if((*pp1)->p_key > (*pp2)->p_key)
		return 1;
	else
		return 0;
}
