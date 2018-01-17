#ifndef	_PROPS_H_
#define	_PROPS_H_

typedef	struct	prop_t	{
	int	p_key;
	char	*p_value;
} PROP_T;

typedef	struct	properties_t	{
	char	*p_fname;
	char	*p_pkey;
	char	*p_hdr;
	int	pf_pkey;
	int	pf_title;
	int	pn_fields;
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
PROPS_new_prop(const char *, const char *);

void
PROPS_delete_prop(PROP_T *);

int
PROPS_read_properties(PROPERTIES_T *);

const PROP_T	*
PROPS_find_props_for_record(PROPERTIES_T *, int);

void
PROPS_dump_properties(FILE *, PROPERTIES_T *);

char	*
PROPS_to_json_object(const PROPERTIES_T *, const PROP_T *);

#endif
