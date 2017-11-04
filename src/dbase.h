#ifndef	_DBASE_H_
#define	_DBASE_H_

#define	DBF_FHDR_SIZE		32
// offsets for fields that we use
#define	DBF_FHDR_VERSION	0
#define	DBF_FHDR_DATE		1
#define	DBF_FHDR_NRECS		4
#define	DBF_FHDR_LHDR		8
#define	DBF_FHDR_LREC		10
#define	DBF_FHDR_ENCRYPTED	15
#define	DBF_FHDR_LANG_INFO	29
#define	DBF_FHDR_TERM		'\x0d'
#define	DBF_REC_GOOD		' '
#define	DBF_REC_DELETED		'*'
#define	DBF_EOF			'\x1a'

#define	DBF_FIELD_SIZE		32
// offsets for fields that we use
#define	DBF_FIELD_NAME		0
#define	DBF_FIELD_TYPE		11
#define	DBF_FIELD_ADDR		12
#define	DBF_FIELD_LENGTH	16
#define	DBF_FIELD_DEC_COUNT	17

typedef	struct	dbf_fdhr_t	{
	int	d_version;
	char	d_date[12];
	int	dn_recs;
	int	dl_hdr;
	int	dl_rec;
	int	d_encrypted;
	int	d_lang_info;
} DBF_FHDR_T;

typedef	struct	dbf_field_t {
	char	d_name[12];
	int	d_type;
	int	d_addr;
	int	d_length;
	int	d_dec_count;
} DBF_FIELD_T;

typedef	struct	dbf_meta_t	{
	char	*d_fname;
	size_t	dl_file;
	DBF_FHDR_T	*d_fhdr;
	int	dn_fields;
	DBF_FIELD_T	**d_fields;
} DBF_META_T;

DBF_META_T	*
DBF_new_dbf_meta(const char *);

void
DBF_delete_dbf_meta(DBF_META_T *);

int
DBF_read_dbf_meta(FILE *, DBF_META_T *);

void
DBF_dump_dbf_meta(FILE *, DBF_META_T *, int);

DBF_FHDR_T	*
DBF_read_fhdr(FILE *);

void
DBF_delete_fhdr(DBF_FHDR_T *);

void
DBF_dump_fhdr(FILE *, DBF_FHDR_T *, const char *);

DBF_FIELD_T	*
DBF_read_field(FILE *);

void
DBF_dump_field(FILE *, DBF_FIELD_T *, int, int, const char *);

void
DBF_dump_rec(FILE *, DBF_META_T *, int, int, int, const char *);

#endif
