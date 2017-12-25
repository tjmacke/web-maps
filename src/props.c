#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "props.h"

static	int
props_cmp_key(const void *, const void *);

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

void
PROPS_delete_all_props(int n_props, PROP_T *props[])
{

	if(props != NULL){
		int	p;

		for(p = 0; p < n_props; p++)
			PROPS_delete_prop(props[p]);
		free(props);
	}
}

int
PROPS_read_propfile(FILE *fp, int *n_props, PROP_T ***props)
{
	char	*line = NULL;
	size_t	s_line;
	ssize_t	l_line;
	int	lcnt, pcnt;
	char	*tab;
	PROP_T	*pp;
	int	err = 0;

	*n_props = 0;
	*props = NULL;

	for(lcnt = pcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0)	// blank line
				continue;
		}
		if(*line == '#')	// comment
			continue;
		pcnt++;
		if((tab = strchr(line, '\t')) == NULL){
			LOG_ERROR("line %7d: no tab", lcnt);
			err = 1;
			goto CLEAN_UP;
		}
		if(tab == line){
			LOG_ERROR("line %7d: empty key field", lcnt);
			err = 1;
			goto CLEAN_UP;
		}
		if(tab[1] == '\0'){
			LOG_ERROR("line %7d: emtpy value field", lcnt);
			err = 1;
			goto CLEAN_UP;
		}
	}
	if(pcnt == 0){
		LOG_ERROR("no properties");
		err = 1;
		goto CLEAN_UP;
	}

	*n_props = pcnt;
	*props = (PROP_T **)calloc(*n_props, sizeof(PROP_T *));
	if(*props == NULL){
		LOG_ERROR("can't allocate props");
		err = 1;
		goto CLEAN_UP;
	}

	rewind(fp);
	for(lcnt = pcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0)	// blank line
				continue;
		}
		if(*line == '#')	// comment
			continue;
		tab = strchr(line, '\t');	// will work, was checked above
		pp = PROPS_new_prop(line, &tab[1]);
		if(pp == NULL){
			LOG_ERROR("line %7d: PROPS_new_prop failed", lcnt);
			err = 1;
			goto CLEAN_UP;
		}
		(*props)[pcnt] = pp;
		pcnt++;
	}
	qsort(*props, *n_props, sizeof(PROP_T *), props_cmp_key);

CLEAN_UP : ;

	if(line != NULL)
		free(line);

	if(err){
		PROPS_delete_all_props(*n_props, *props);
		*n_props = 0;
		*props = NULL;
	}

	return err;
}

const PROP_T	*
PROPS_find_prop(int pkey, int n_props, PROP_T *props[])
{
	int	i, j, k;
	PROP_T	*pp;

	for(i = 0, j = n_props - 1; i <= j; ){
		k = (i + j) / 2;
		pp = props[k];
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
PROPS_dump_props(FILE *fp, int n_props, PROP_T *props[])
{
	int	p;
	PROP_T	*pp;

	if(props == NULL){
		fprintf(fp, "props = NULL\n");
		return;
	}

	fprintf(fp, "props = %d {\n", n_props);
	for(p = 0; p < n_props; p++){
		pp = props[p];
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
