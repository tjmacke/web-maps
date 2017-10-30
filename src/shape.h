#ifndef	_SHAPE_H_
#define	_SHAPE_H_

// Add more as needed, but these 4 are required in the general case
#define	SFT_UNKNOWN	0
#define	SFT_SHP		1
#define	SFT_SHX		2
#define	SFT_DBF		3
#define	SFT_PRJ		4

#define	SF_MAGIC	9994
#define	SF_WORD_SIZE	2	// shape file lengths and offset are as 16 bit words (2 bytes)
#define	SF_VERSION	1000
#define	SF_NO_DATA_UB	-1e38	// values smaller are conisded to be "no data"

// shape types range from [0,33], although only 14 types were ever assigned  
#define	ST_NULL		0
#define ST_POINT	1
#define	ST_POLYLINE	3
#define	ST_POLYGON	5
#define	ST_MULTIPOINT	8
#define	ST_POINT_Z	11
#define	ST_POLYLINE_Z	13
#define	ST_POLYGON_Z	15
#define	ST_MULTIPOINT_Z	18
#define	ST_POINT_M	21
#define	ST_POLYLINE_M	23
#define	ST_POLYGON_M	25
#define	ST_MULTIPOINT_M	28
#define	ST_MULTIPATH	31
#define	ST_LAST		33

typedef	struct	sf_bbox_t	{
	double	s_xmin;
	double	s_ymin;
	double	s_xmax;
	double	s_ymax;
} SF_BBOX_T;

typedef	struct	sf_fhdr_t	{
	int	s_magic;
	int	sl_file;
	int	s_version;
	int	s_type;
	SF_BBOX_T	s_bbox;
	double	s_zmin;
	double	s_zmax;
	double	s_mmin;
	double	s_mmax;
} SF_FHDR_T;

int
SHP_get_file_type(const char *);

int
SHP_read_fhdr(FILE *, SF_FHDR_T *);

int
SHP_read_bbox(FILE *, SF_BBOX_T *);

void
SHP_dump_fhdr(FILE *, SF_FHDR_T *);

#endif
