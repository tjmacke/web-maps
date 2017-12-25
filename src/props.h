#ifndef	_PROPS_H_
#define	_PROPS_H_

typedef	struct	prop_t	{
	int	p_key;
	char	*p_value;
} PROP_T;

PROP_T	*
PROPS_new_prop(const char *, const char *);

int
PROPS_read_propfile(FILE *, int *, PROP_T ***);

void
PROPS_dump_props(FILE *, int, PROP_T **);

void
PROPS_delete_prop(PROP_T *);

void
PROPS_delete_all_props(int, PROP_T *[]);

#endif
