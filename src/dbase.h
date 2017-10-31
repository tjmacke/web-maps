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

typedef	struct	dbase_t	{
	DBF_FHDR_T	*d_fhdr;
} DBASE_T;

DBF_FHDR_T	*
DBF_new_fhdr(void);

int
DBF_read_fhdr(FILE *, DBF_FHDR_T *);

void
DBF_dump_fhdr(FILE *, DBF_FHDR_T *);

void
DBF_delete_fhdr(DBF_FHDR_T *);

#endif
