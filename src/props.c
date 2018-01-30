#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "props.h"

static	int
props_cmp_int_key(const void *, const void *);

static	int
props_cmp_str_key(const void *, const void *);

static	char	*
mk_json_kv_pair(const char *, const char *, int);

static	char	*
mk_json_primary(const char *, int);

static	size_t
fi_json_strlen(const char *);

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

PROPERTIES_T	*
PROPS_make_default_ptab(const char *pkey)
{
	PROPERTIES_T	*props = NULL;
	int	err = 0;

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
	props->p_pkey = strdup(pkey);
	if(props->p_pkey == NULL){
		LOG_ERROR("can't strdup pkey %s", pkey);
		err = 1;
		goto CLEAN_UP;
	}
	props->pn_ftab = 1;
	props->p_ftab = (char **)calloc((size_t)1, sizeof(char *));
	if(props->p_ftab == NULL){
		LOG_ERROR("can't allocate p_ftab");
		err = 1;
		goto CLEAN_UP;
	}
	props->p_ftab[0] = strdup(pkey);
	if(props->p_ftab[0] == NULL){
		LOG_ERROR("can't strdup pkey");
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
	int	i;

	if(props == NULL)
		return;

	if(props->p_ptab != NULL){
		for(i = 0; i < props->pn_ptab; i++){
			if(props->p_ptab[i] != NULL)
				PROPS_delete_prop(props->p_ptab[i]);
		}
		free(props->p_ptab);
	}

	if(props->p_ftab != NULL){
		for(i = 0; i < props->pn_ftab; i++){
			if(props->p_ftab[i] != NULL)
				free(props->p_ftab[i]);
		}
		free(props->p_ftab);
	}

	if(props->p_pkey != NULL)
		free(props->p_pkey);

	if(props->p_fname != NULL)
		free(props->p_fname);

	free(props);
}

PROP_T	*
PROPS_new_prop(const char *key, int int_key, const char *value)
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

	pp = (PROP_T *)calloc((size_t)1, sizeof(PROP_T));
	if(pp == NULL){
		LOG_ERROR("can't allocate pp");
		err = 1;
		goto CLEAN_UP;
	}
	if(int_key)
		pp->p_int_key = atoi(key);
	else{
		pp->p_str_key = strdup(key);
		if(pp->p_str_key == NULL){
			LOG_ERROR("can't strdup key");
			err = 1;
			goto CLEAN_UP;
		}
	}
	pp->p_value = strdup(value);
	if(pp->p_value == NULL){
		LOG_ERROR("can't strdup value");
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
		if(pp->p_str_key != NULL)
			free(pp->p_str_key);
		free(pp);
	}
}

int
PROPS_read_properties(PROPERTIES_T *props, int int_key)
{
	FILE	*fp = NULL;
	char	*line = NULL;
	size_t	s_line;
	ssize_t	l_line;
	int	lcnt;
	int	nf;
	char	*s_fp, *e_fp;
	char	*sp = NULL;
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

	props->pn_ptab = (lcnt - 1);	// first line is the header and is stored in p_ftab
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
		if(lcnt == 1){	// the header
			// get the number of fields, set pf_pkey, pf_title
			for(nf = 0, e_fp = s_fp = line; *s_fp; ){
				nf++;
				if((e_fp = strchr(s_fp, '\t')) == NULL)
					e_fp = s_fp + strlen(s_fp);
				if(e_fp == s_fp){
					LOG_ERROR("header field %d is empty", nf);
					err = 1;
					goto CLEAN_UP;
				}
				// deal with empty last field
				if(*e_fp == '\t' && e_fp[1] == '\0'){
					nf++;
					LOG_ERROR("header field %d (last) is empty", nf);
					err = 1;
					goto CLEAN_UP;
				}
				s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
			}
			props->pn_ftab = nf;
			props->p_ftab = (char **)calloc((size_t)nf, sizeof(char *));
			if(props->p_ftab == NULL){
				LOG_ERROR("can't allocate p_ftab");
				err = 1;
				goto CLEAN_UP;
			}
			// store the field names in p_ftab. No empty fields at this point
			for(nf = 0, e_fp = s_fp = line; *s_fp; nf++){
				char	*sp = NULL;

				if((e_fp = strchr(s_fp, '\t')) == NULL)
					e_fp = s_fp + strlen(s_fp);
				sp = strndup(s_fp, e_fp - s_fp);
				if(sp == NULL){
					LOG_ERROR("can't allocate header field %d", nf+1);
					err = 1;
					goto CLEAN_UP;
				}
				if(!strcmp(sp, props->p_pkey))
					props->pf_pkey = nf+1;
				else if(!strcmp(sp, "title"))
					props->pf_title = nf+1;
				props->p_ftab[nf] = sp;
				sp = NULL;
				s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
			}
			if(props->pf_pkey == 0){
				LOG_ERROR("pkey field %s not in header", props->p_pkey);
				err = 1;
				goto CLEAN_UP;
			}
			if(!strcmp(props->p_pkey, "title"))
				props->pf_title = props->pf_pkey;
			if(props->pf_title == 0){
				LOG_ERROR("title field %s not in header", "title");
				err = 1;
				goto CLEAN_UP;
			}
		}else{	// the property records
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
			if(nf != props->pn_ftab){
				LOG_ERROR("line %d: wrong number of fields %d, must be %d", lcnt, nf, props->pn_ftab);
				err = 1;
				goto CLEAN_UP;
			}
			pp = PROPS_new_prop(pkey, int_key, line);
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
	qsort(props->p_ptab, props->pn_ptab, sizeof(PROP_T *), int_key ? props_cmp_int_key : props_cmp_str_key);

CLEAN_UP : ;

	if(pkey != NULL)
		free(pkey);

	if(line != NULL)
		free(line);

	if(fp != NULL)
		fclose(fp);

	return err;
}

const PROP_T	*
PROPS_find_props_with_int_key(const PROPERTIES_T *props, int pkey)
{
	int	i, j, k;
	PROP_T	*pp;

	for(i = 0, j = props->pn_ptab - 1; i <= j; ){
		k = (i + j) / 2;
		pp = props->p_ptab[k];
		if(pp->p_int_key == pkey)
			return pp;
		else if(pp->p_int_key < pkey)
			i = k + 1;
		else
			j = k - 1;
	}
	return NULL;
}

const PROP_T	*
PROPS_find_props_with_str_key(const PROPERTIES_T *props, const char *pkey)
{
	int	i, j, k, cv;
	PROP_T	*pp;

	for(i = 0, j = props->pn_ptab - 1; i <= j; ){
		k = (i + j) / 2;
		pp = props->p_ptab[k];
		if((cv = strcmp(pp->p_str_key, pkey)) == 0)
			return pp;
		else if(cv < 0)
			i = k + 1;
		else
			j = k - 1;
	}
	return NULL;
}

void
PROPS_dump_properties(FILE *fp, const PROPERTIES_T *props)
{
	int	i;
	PROP_T	*pp;

	if(props == NULL){
		fprintf(fp, "props = NULL\n");
		return;
	}

	fprintf(fp, "props = %d {\n", props->pn_ptab);
	fprintf(fp, "\tftab = %d {\n", props->pn_ftab);
	for(i = 0; i < props->pn_ftab; i++)
		fprintf(fp, "\t\t%d = %s\n", i+1, props->p_ftab[i]);
	fprintf(fp, "\t}\n");
	fprintf(fp, "\tptab = %d {\n", props->pn_ptab);
	for(i = 0; i < props->pn_ptab; i++){
		pp = props->p_ptab[i];
		if(pp == NULL)
			fprintf(fp, "\t\t%d = NULL\n", i+1);
		else{
			fprintf(fp, "\t\t%d = {\n", i+1);
			fprintf(fp, "\t\t\tint_key   = %d\n", pp->p_int_key);
			fprintf(fp, "\t\t\tstr_key   = %s\n", pp->p_str_key != NULL ? pp->p_str_key : "NULL");
			fprintf(fp, "\t\t\tvalue     = %s\n", pp->p_value ? pp->p_value : "NULL");
			fprintf(fp, "\t\t}\n");
		}
	}
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");
}

char	*
PROPS_to_json_object(const PROPERTIES_T *props, const PROP_T *pp)
{
	char	**values = NULL;
	char	**kvp_tab = NULL;
	int	i;
	char	*s_fp, *e_fp;
	char	*sp = NULL;
	size_t	s_obj = 0;
	char	*obj = NULL;
	char	*op;
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

	values = (char **)calloc((size_t)props->pn_ftab, sizeof(char *));
	if(values == NULL){
		LOG_ERROR("can't allocate values");
		err = 1;
		goto CLEAN_UP;
	}

	kvp_tab = (char **)calloc((size_t)props->pn_ftab, sizeof(char *));
	if(kvp_tab == NULL){
		LOG_ERROR("can't allocate kvp_tab");
		err = 1;
		goto CLEAN_UP;
	}

	// get the values from pp->p_value
	for(i = 0, e_fp = s_fp = pp->p_value; *s_fp; ){
		if((e_fp = strchr(s_fp, '\t')) == NULL)
			e_fp = s_fp + strlen(s_fp);

		// handle empty field
		sp = e_fp == s_fp ? strdup("") : strndup(s_fp, e_fp - s_fp);
		if(sp == NULL){
			LOG_ERROR("can't strdup value field %d, \"%.*s\"", i+1, (int)(e_fp - s_fp), s_fp);
			err = 1;
			goto CLEAN_UP;
		}
		values[i] = sp;
		sp = NULL;
		i++;

		// check for and handle empty last field
		if(*e_fp == '\t' && e_fp[1] == '\0'){
			sp = strdup("");
			if(sp == NULL){
				LOG_ERROR("can't strdup empty field %d (last), \"\"", i+1);
				err = 1;
				goto CLEAN_UP;
			}
			values[i] = sp;
			sp = NULL;
			i++;
		}

		s_fp = *e_fp != '\0' ? e_fp + 1 : e_fp;
	}

	s_obj = 1;	// {
	for(i = 0; i < props->pn_ftab; i++){
		char	*kvp = mk_json_kv_pair(props->p_ftab[i], values[i], 0);
		if(kvp == NULL){
			LOG_ERROR("mk_json_kv_pair failed");
			err = 1;
			goto CLEAN_UP;
		}
		s_obj += strlen(kvp);
		kvp_tab[i] = kvp;
		kvp = NULL;
	}
	s_obj += 2 * (props->pn_ftab - 1) + 1 + 1;	// ", " between kv pairs, closing } and \0
	obj = (char *)malloc(s_obj * sizeof(char));
	if(obj == NULL){
		LOG_ERROR("can't allocate obj");
		err = 1;
		goto CLEAN_UP;
	}
	op = obj;
	*op++ = '{';
	for(i = 0; i < props->pn_ftab; i++){
		strcpy(op, kvp_tab[i]);
		op += strlen(kvp_tab[i]);
		if(i < props->pn_ftab - 1){
			strcpy(op, ", ");
			op += 2;
		}
	}
	*op++ = '}';
	*op = '\0';

CLEAN_UP : ;

	if(sp != NULL)
		free(sp);

	if(kvp_tab != NULL){
		for(i = 0; i < props->pn_ftab; i++){
			if(kvp_tab[i] != NULL)
				free(kvp_tab[i]);
		}
		free(kvp_tab);
	}

	if(values != NULL){
		for(i = 0; i < props->pn_ftab; i++){
			if(values[i] != NULL)
				free(values[i]);
		}
		free(values);
	}

	if(err){
		if(obj != NULL)
			free(obj);
		obj = NULL;
	}

	return obj;
}

static	int
props_cmp_int_key(const void *p1, const void *p2)
{
	const PROP_T	**pp1 = (const PROP_T **)p1;
	const PROP_T	**pp2 = (const PROP_T **)p2;

	if((*pp1)->p_int_key < (*pp2)->p_int_key)
		return -1;
	else if((*pp1)->p_int_key > (*pp2)->p_int_key)
		return 1;
	else
		return 0;
}

static	int
props_cmp_str_key(const void *p1, const void *p2)
{
	const PROP_T	**pp1 = (const PROP_T **)p1;
	const PROP_T	**pp2 = (const PROP_T **)p2;

	return strcmp((*pp1)->p_str_key, (*pp2)->p_str_key);
}

static	char	*
mk_json_kv_pair(const char *key, const char *value, int is_str)
{
	char	*j_key = NULL;
	char	*j_value = NULL;
	size_t	s_kvp;
	char	*kvp = NULL;
	int	err = 0;

	j_key = mk_json_primary(key, 1);
	j_value = mk_json_primary(value, is_str);
	s_kvp = strlen(j_key) + 2 + strlen(j_value) + 1;
	kvp = (char *)malloc(s_kvp * sizeof(char));
	if(kvp == NULL){
		LOG_ERROR("can't allocate kvp");
		err = 1;
		goto CLEAN_UP;
	}
	sprintf(kvp, "%s: %s", j_key, j_value);

CLEAN_UP : ;

	if(j_value != NULL)
		free(j_value);

	if(j_key != NULL)
		free(j_key);

	if(err){
		if(kvp != NULL){
			free(kvp);
			kvp = NULL;
		}
	}

	return kvp;
}

static	char	*
mk_json_primary(const char *str, int is_str)
{
	const char	*sp;
	size_t	jl_str = 0;
	char	*j_str = NULL;
	char	*jsp;
	int	err = 0;

	if(!is_str){
		// check for 3 json reserved words: null, false, true, if no match could be a number
		if(strcmp(str, "null") && strcmp(str, "false") && strcmp(str, "true")){	// possible number
			sp = str;
			if(*sp == '-')
				sp++;
			if(isdigit(*sp)){	// still possible number
				if(*sp == '0')
					sp++;
				else{
					for(++sp; isdigit(*sp); sp++)
						;
				}
				if(*sp == '.'){		// possible real
					sp++;
					if(isdigit(*sp)){	// still possible real
						for(++sp; isdigit(*sp); sp++)
							;
						if(*sp == 'e' && *sp == 'E'){	// possible exponent
							sp++;
							if(*sp == '+' || *sp == '-')
								sp++;
							if(isdigit(*sp)){	// still possible exponent
								for(++sp; isdigit(*sp); sp++)
									;
								is_str = *sp != '\0';	// NUM if at end else STR
							}else
								is_str = 1;
						}else if(*sp != '\0')	// STR: frac digits followed by other than exponent
							is_str = 1;
					}else
						is_str = 1;
				}else if(*sp != '\0')	// STR: digits followed by something other than '.'
					is_str = 1;
			}else	// STR: - followed by non-digit
				is_str = 1;
		}
	}

	if(is_str){
		jl_str = fi_json_strlen(str);
		jl_str += 2 + 1;		// "<str>"\0
		j_str = (char *)malloc(jl_str * sizeof(char));
		if(j_str == NULL){
			LOG_ERROR("can't allocate j_str");
			err = 1;
			goto CLEAN_UP;
		}
		jsp = j_str;
		*jsp++ = '"';
		for(sp = str; *sp; sp++){
			if((*sp & 0xff) < 32){
				sprintf(jsp, "\\u%04x", (*sp & 0xff));
				jsp += 6;
			}else{
				switch(*sp){
				case '"' :
				case '\\' :
				case '/' :
					*jsp++ = '\\';
					*jsp++ = *sp;
					break;
				case '\b' :
					*jsp++ = '\\';
					*jsp++ = 'b';
					break;
				case '\f' :
					*jsp++ = '\\';
					*jsp++ = 'f';
					break;
				case '\n' :
					*jsp++ = '\\';
					*jsp++ = 'n';
					break;
				case '\r' :
					*jsp++ = '\\';
					*jsp++ = 'r';
					break;
				case '\t' :
					*jsp++ = '\\';
					*jsp++ = 't';
					break;
				default :
					*jsp++ = *sp;
				}
			}
		}
		*jsp++ = '"';
		*jsp = '\0';
	}else{	// just dup it
		j_str = strdup(str);	
		if(j_str == NULL){
			LOG_ERROR("can't strdup str");
			err = 1;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;

	if(err){
		if(j_str != NULL){
			free(j_str);
			j_str = NULL;
		}
	}

	return j_str;
}

static	size_t
fi_json_strlen(const char *str)
{
	size_t	l_str;
	const char	*sp;

	for(l_str = 2, sp = str; *sp; sp++){
		if(*sp == '"' || *sp == '\\' || *sp == '/')
			l_str++;
		else if(*sp == '\b' || *sp == '\f' || *sp == '\n' || *sp == '\r' || *sp == '\t')
			l_str++;
		else if((*sp & 0xff) < 32)
			l_str += 5;
		l_str++;
	}
	return l_str;
}
