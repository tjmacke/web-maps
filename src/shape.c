#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "shape.h"

static	int
read_be_int(FILE *, int *);

static	int
read_le_int(FILE *, int *);

static	int
read_le_double(FILE *, double *);

int
SHP_get_file_type(const char *fname)
{
	int	ftype = SFT_UNKNOWN;
	const char	*fnp;
	const char	*slash, *dot;
	int	err = 0;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	for(slash = dot = NULL, fnp = fname; *fnp; fnp++){
		if(*fnp == '/')
			slash = fnp;
		else if(*fnp == '.')
			dot = fnp;
	}

	// find the extension
	if(dot == NULL){
		LOG_ERROR("no extension: %s", fname);
		err = 1;
		goto CLEAN_UP;
	}else if(slash){
		if(dot < slash){	 // last dot is a directory
			LOG_ERROR("no extension: %s", fname);
			err = 1;
			goto CLEAN_UP;
		}
	}
	dot++;
	if(!strcmp(dot, "shp"))
		ftype = SFT_SHP;
	else if(!strcmp(dot, "shx"))
		ftype = SFT_SHX;
	else if(!strcmp(dot, "dbf"))
		ftype =  SFT_DBF;
	else if(!strcmp(dot, "prj"))
		ftype = SFT_PRJ;
	else{
		LOG_ERROR("unknown file type '%s'", dot);
		err = 1;
		goto CLEAN_UP;
	}


CLEAN_UP : ;

	if(err)
		ftype = SFT_UNKNOWN;

	return ftype;
}

int
SHP_read_fhdr(FILE *fp, SF_FHDR_T *fhdr)
{
	int	ival, i, c;
	double	dval;
	int	err = 0;

	memset(fhdr, 0, sizeof(SF_FHDR_T));

	// values 1-7 are big endian
	// read the magic number
	if(read_be_int(fp, &ival)){
		LOG_ERROR("read_be_int failed for magic number");
		err = 1;
		goto CLEAN_UP;
	}
	if(ival != SF_MAGIC){
		LOG_ERROR("bad magic number %d, need %d", ival, SF_MAGIC);
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->s_magic = ival;

	// The next 4 ints (20 bytes) must be 0, but are not used
	for(i = 0; i < 5; i++){
		if(read_be_int(fp, &ival)){
			LOG_ERROR("read_be_int failed for unused value %d", i+1);
			err = 1;
			goto CLEAN_UP;
		}
		if(ival != 0){
			LOG_ERROR("bad unused value %d: %d, must be 0", i+1, ival);
			err = 1;
			goto CLEAN_UP;
		}
	}

	// get the file length
	if(read_be_int(fp, &ival)){
		LOG_ERROR("read_be_int failed for file length");
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->sl_file = ival;	// 2 byte words!

	// the remaining values are little endian
	// get the version
	if(read_le_int(fp, &ival)){
		LOG_ERROR("read_le_int failed for version");
		err = 1;
		goto CLEAN_UP;
	}
	if(ival != SF_VERSION){
		LOG_ERROR("bad version %d, must be %d", ival, SF_VERSION);
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->s_version = ival;

	// get the shape type
	if(read_le_int(fp, &ival)){
		LOG_ERROR("read_le_int failed for version");
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->s_type = ival;

	if(SHP_read_bbox(fp, &fhdr->s_bbox)){
		LOG_ERROR("SHP_get_bbox failed");
		err = 1;
		goto CLEAN_UP;
	}

	// get the z range
	if(read_le_double(fp, &fhdr->s_zmin)){
		LOG_ERROR("read_le_double failed for zmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(read_le_double(fp, &fhdr->s_zmax)){
		LOG_ERROR("read_le_double failed for zmax");
		err = 1;
		goto CLEAN_UP;
	}

	// get the m range
	if(read_le_double(fp, &fhdr->s_mmin)){
		LOG_ERROR("read_le_double failed for mmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(read_le_double(fp, &fhdr->s_mmax)){
		LOG_ERROR("read_le_double failed for mmax");
		err = 1;
		goto CLEAN_UP;
	}


CLEAN_UP : ;

	// prevent info leak
	if(err)
		memset(fhdr, 0, sizeof(SF_FHDR_T));

	return err;
}

int
SHP_read_bbox(FILE *fp, SF_BBOX_T *bbox)
{
	int	err = 0;

	memset(bbox, 0, sizeof(SF_BBOX_T));

	if(read_le_double(fp, &bbox->s_xmin)){
		LOG_ERROR("read_le_double failed for xmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(read_le_double(fp, &bbox->s_ymin)){
		LOG_ERROR("read_le_double failed for ymin");
		err = 1;
		goto CLEAN_UP;
	}
	if(read_le_double(fp, &bbox->s_xmax)){
		LOG_ERROR("read_le_double failed for xmax");
		err = 1;
		goto CLEAN_UP;
	}
	if(read_le_double(fp, &bbox->s_ymax)){
		LOG_ERROR("read_le_double failed for ymax");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	// prevent info leak on err
	if(err)
		memset(bbox, 0, sizeof(SF_BBOX_T));

	return err;
}

void
SHP_dump_fhdr(FILE *fp, SF_FHDR_T *fhdr)
{

	if(fhdr == NULL){
		fprintf(fp, "fhdr is NULL\n");
		return;
	}

	fprintf(fp, "fhdr = {\n");
	fprintf(fp, "\tmagic   = %d\n", fhdr->s_magic);
	fprintf(fp, "\tl_file  = %d\n", fhdr->sl_file);
	fprintf(fp, "\tversion = %d\n", fhdr->s_version);
	fprintf(fp, "\ttype    = %d\n", fhdr->s_type);
	fprintf(fp, "\bbox     = {\n");
	fprintf(fp, "\t\txmin   = %.15e\n", fhdr->s_bbox.s_xmin);
	fprintf(fp, "\t\tymin   = %.15e\n", fhdr->s_bbox.s_ymin);
	fprintf(fp, "\t\txmax   = %.15e\n", fhdr->s_bbox.s_xmax);
	fprintf(fp, "\t\tymax   = %.15e\n", fhdr->s_bbox.s_ymax);
	fprintf(fp, "\t}\n");
	fprintf(fp, "\tzmin    = %.15e\n", fhdr->s_zmin);
	fprintf(fp, "\tzmax    = %.15e\n", fhdr->s_zmax);
	fprintf(fp, "\tmmin    = %.15e\n", fhdr->s_mmin);
	fprintf(fp, "\tmmax    = %.15e\n", fhdr->s_mmax);
	fprintf(fp, "}\n");
}

static	int
read_be_int(FILE *fp, int *ival)
{
	int	i, c;
	int	err = 0;

	for(*ival = i = 0; i < 4; i++){
		if((c = getc(fp)) == EOF){
			LOG_ERROR("short integer: %d bytes, need 4", i);
			err = 1;
			goto CLEAN_UP;
		}
		*ival = (*ival << 8) | (c & 0xff);
	}

CLEAN_UP : ;

	if(err)
		*ival = 0;

	return err;
}

static	int
read_le_int(FILE *fp, int *ival)
{
	int	i, c;
	int	err = 0;

	for(*ival = i = 0; i < 4; i++){
		if((c = getc(fp)) == EOF){
			LOG_ERROR("short integer: %d bytes, need 4", i);
			err = 1;
			goto CLEAN_UP;
		}
		*ival = *ival | ((c & 0xff) << (i * 8));
	}

CLEAN_UP : ;

	if(err)
		*ival = 0;

	return err;
}

static	int
read_le_double(FILE *fp, double *dval)
{
	int	i, c;
	union {
		char	v_str[8];
		double	v_double;
	} c2d;
	int	err = 0;

	*dval = 0;

	for(c2d.v_double = 0, i = 0; i < 8; i++){
		if((c = getc(fp)) == EOF){
			LOG_ERROR("short double: %d bytes, need 8", i);
			err = 1;
			goto CLEAN_UP;
		}
		c2d.v_str[i] = (c & 0xff);
	}
	*dval = c2d.v_double;

CLEAN_UP : ;

	if(err)
		*dval = 0;

	return err;
}
