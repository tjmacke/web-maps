#ifndef	_SHAPE_H_
#define	_SHAPE_H_

// Add more as needed, but these 4 are required in the general case
#define	SFT_UNKNOWN	0
#define	SFT_SHP		1
#define	SFT_SHX		2
#define	SFT_DBF		3
#define	SFT_PRJ		4

int
SHP_get_file_type(const char *);

#endif
