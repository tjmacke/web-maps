#ifndef	_ADDRS_H_
#define	_ADDRS_H_

#define	AT_POINT	0
#define	AT_SIGN		1
#define	AT_BOXED_PT	2

typedef	struct	addr_t	{
	int	a_type;
	int	a_lnum;
	char	*a_line;
	// the parsed query
	char	*a_date;
	char	*a_qry;
	double	a_lng;
	double	a_lat;
	char	*a_rply;
} ADDR_T;

typedef	struct	adata_t	{
	char	*a_fname;
	FILE	*a_fp;
	int	as_atab;
	int	an_atab;
	ADDR_T	**a_atab;
} ADATA_T;

ADATA_T	*
AD_new_adata(const char *, int);

void
AD_delete_adata(ADATA_T *);

int
AD_read_adata(ADATA_T *, int, int);

void
AD_dump_adata(FILE *, const ADATA_T *, int);

ADDR_T	*
AD_new_addr(const char *, int, int);

void
AD_delete_addr(ADDR_T *);

void
AD_print_addr(FILE *, const ADDR_T *, const char *);

#endif
