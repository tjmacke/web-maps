#ifndef	_PROPS_H_
#define	_PROPS_H_

typedef	struct	prop_t	{
	int	p_int_key;
	char	*p_str_key;
	char	*p_value;
} PROP_T;

typedef	struct	properties_t	{
	char	*p_fname;
	char	*p_pkey;
	int	pf_pkey;
	int	pf_title;
	int	pn_ftab;
	char	**p_ftab;
	int	pn_ptab;
	PROP_T	**p_ptab;
} PROPERTIES_T;

PROPERTIES_T	*
PROPS_new_properties(const char *, const char *);

void
PROPS_delete_properties(PROPERTIES_T *);

void
PROPS_dump_properties(FILE *, PROPERTIES_T *);

PROP_T	*
PROPS_new_prop(const char *, int, const char *);

void
PROPS_delete_prop(PROP_T *);

int
PROPS_read_properties(PROPERTIES_T *, int);

const PROP_T	*
PROPS_find_props_with_int_key(PROPERTIES_T *, int);

const PROP_T	*
PROPS_find_props_with_str_key(PROPERTIES_T *, const char *);

void
PROPS_dump_properties(FILE *, PROPERTIES_T *);

char	*
PROPS_to_json_object(const PROPERTIES_T *, const PROP_T *);

#endif
