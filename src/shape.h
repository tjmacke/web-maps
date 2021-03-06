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
#define	SF_NO_DATA_TJM	-2e38	// smaller than above, so I'm using this in case I need a no data avlue.

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
#define	ST_MULTIPATCH	31
#define	ST_LAST		33

// part types for multipatch
#define	STP_TRI_STRIP	0
#define	STP_TRI_FAN	1
#define	STP_OUTER_RING	2
#define	STP_INNER_RING	3
#define	STP_FIRST_RING	4
#define	STP_RING	5

// macros that chk type stuff
#define	ST_IS_POINT_TYPE(t)	(((t) == ST_POINT) || ((t) == ST_POINT_Z) || ((t) == ST_POINT_M))
#define	ST_IS_MPOINT_TYPE(t)	(((t) == ST_MULTIPOINT) || ((t) == ST_MULTIPOINT_Z) || ((t) == ST_MULTIPOINT_M))
#define	ST_IS_PLINE_TYPE(t)	(((t) == ST_POLYLINE) || ((t) == ST_POLYLINE_Z) || ((t) == ST_POLYLINE_M))
#define	ST_IS_PGON_TYPE(t)	(((t) == ST_POLYGON) || ((t) == ST_POLYGON_Z) || ((t) == ST_POLYGON_M))
#define	ST_HAS_ZDATA(t)		\
	((t) == ST_POINT_Z || ((t) == ST_MULTIPOINT_Z) || ((t) == ST_POLYLINE_Z) || ((t) == ST_POLYGON_Z) || ((t) == ST_MULTIPATCH))
#define	ST_HAS_MDATA(t)		\
	(ST_HAS_ZDATA(t) || ((t) == ST_POINT_M) || ((t) == ST_MULTIPOINT_M) || ((t) == ST_POLYLINE_M) || ((t) == ST_POLYGON_M))
#define	ST_HAS_PARTS(t)	\
	(((t) == ST_POLYLINE) || ((t) == ST_POLYLINE_Z) || ((t) == ST_POLYLINE_M) ||	\
	 ((t) == ST_POLYGON) || ((t) == ST_POLYGON_Z) || ((t) == ST_POLYGON_M) ||		\
	 ((t) == ST_MULTIPATCH))

#define	SF_FHDR_SIZE	50	// 50 words = 100 bytes
#define	SF_RIDX_SIZE	4	// 4 words = 8 bytes

#define	GJ_DEFAULT_TITLE_FMT_D	"shape_%07d"
#define	GJ_DEFAULT_PROPS_FMT_D	"{\"title\": \"shape_%07d\"}"

typedef	struct	sf_bbox_t	{
	double	s_xmin;
	double	s_ymin;
	double	s_xmax;
	double	s_ymax;
} SF_BBOX_T;

typedef	struct	sf_fhdr_t	{
	char	*s_fname;
	FILE	*s_fp;
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

typedef	struct	sf_ridx_t	{
	int	s_offset;	// 16 bit words!
	int	s_length;	// 16 bit words!
} SF_RIDX_T;

typedef	struct	sf_point_t	{
	double	s_x;
	double	s_y;
} SF_POINT_T;

typedef	struct	sf_edge_t	{
	int	s_name_idx;
	int	s_pnum;
	int	s_is_vertical;
	double	s_slope;
	double	s_intercept;
	double	s_x1;
	double	s_y1;
	double	s_x2;
	double	s_y2;
} SF_EDGE_T;

typedef	struct	sf_shape_t	{
	int	s_rnum;
	int	s_length;	// 16 bit words!
	int	s_type;	
	SF_BBOX_T	s_bbox;
	int	sn_parts;
	int	sn_points;
	int	*s_parts;
	int	*s_ptypes;
	SF_POINT_T	*s_points;
	double	s_zmin;
	double	s_zmax;
	double	*s_zvals;
	double	s_mmin;
	double	s_mmax;
	double	*s_mvals;
} SF_SHAPE_T;

int
SHP_get_file_type(const char *);

char	*
SHP_make_sf_name(const char *, const char *);

SF_FHDR_T	*
SHP_new_fhdr(const char *);

void
SHP_write_fhdr(FILE *, const SF_FHDR_T *);

void
SHP_delete_fhdr(SF_FHDR_T *);

SF_FHDR_T	*
SHP_open_file(const char *);

void
SHP_close_file(SF_FHDR_T *);

int
SHP_read_bbox(FILE *, SF_BBOX_T *);

void
SHP_write_bbox(FILE *, const SF_BBOX_T *);

void
SHP_dump_bbox(FILE *, const SF_BBOX_T *, const char *);

int
SHP_read_point(FILE *, SF_POINT_T *);

void
SHP_dump_point(FILE *, const SF_POINT_T *, const char *, int);

int
SHP_read_ridx(FILE *, SF_RIDX_T *);

void
SHP_write_ridx(FILE *, const SF_RIDX_T *);

void
SHP_dump_fhdr(FILE *, SF_FHDR_T *);

void
SHP_dump_ridx(FILE *, SF_RIDX_T *);

int
SHP_read_shx_data(const char *, int, int *, SF_RIDX_T **);

SF_SHAPE_T	*
SHP_read_shape(FILE *);

void
SHP_delete_shape(SF_SHAPE_T *);

void
SHP_dump_shape(FILE *, SF_SHAPE_T *, int);

int
SHP_write_geojson_prolog(FILE *, const char *, const char *);

int
SHP_write_geojson(FILE *, const SF_SHAPE_T *, int, const char *);

void
SHP_write_geojson_trailer(FILE *, const char *);

#endif
