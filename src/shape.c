#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "file_io.h"
#include "shape.h"

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

SF_FHDR_T	*
SHP_new_fhdr(const char *fname)
{
	SF_FHDR_T	*fhdr = NULL;
	int	err = 0;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	fhdr = (SF_FHDR_T *)calloc((size_t)1, sizeof(SF_FHDR_T));
	if(fhdr == NULL){
		LOG_ERROR("can't allocate fhdr");
		err = 1;
		goto CLEAN_UP;
	}

	fhdr->s_fname = strdup(fname);
	if(fhdr->s_fname == NULL){
		LOG_ERROR("can't strdup fname");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(err){
		SHP_delete_fhdr(fhdr);
		fhdr = NULL;
	}

	return fhdr;
}

void
SHP_delete_fhdr(SF_FHDR_T *fhdr)
{

	if(fhdr == NULL)
		return;

	if(fhdr->s_fname != NULL)
		free(fhdr->s_fname);

	free(fhdr);
}

int
SHP_read_fhdr(FILE *fp, SF_FHDR_T *fhdr)
{
	int	ival, i, c;
	double	dval;
	int	err = 0;

	// values 1-7 are big endian
	// read the magic number
	if(FIO_read_be_int4(fp, &ival)){
		LOG_ERROR("FIO_read_be_int4 failed for magic number");
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
		if(FIO_read_be_int4(fp, &ival)){
			LOG_ERROR("FIO_read_be_int4 failed for unused value %d", i+1);
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
	if(FIO_read_be_int4(fp, &ival)){
		LOG_ERROR("FIO_read_be_int4 failed for file length");
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->sl_file = ival;	// 2 byte words!

	// the remaining values are little endian
	// get the version
	if(FIO_read_le_int4(fp, &ival)){
		LOG_ERROR("FIO_read_le_int4 failed for version");
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
	if(FIO_read_le_int4(fp, &ival)){
		LOG_ERROR("FIO_read_le_int4 failed for version");
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
	if(FIO_read_le_double(fp, &fhdr->s_zmin)){
		LOG_ERROR("FIO_read_le_double failed for zmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fp, &fhdr->s_zmax)){
		LOG_ERROR("FIO_read_le_double failed for zmax");
		err = 1;
		goto CLEAN_UP;
	}

	// get the m range
	if(FIO_read_le_double(fp, &fhdr->s_mmin)){
		LOG_ERROR("FIO_read_le_double failed for mmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fp, &fhdr->s_mmax)){
		LOG_ERROR("FIO_read_le_double failed for mmax");
		err = 1;
		goto CLEAN_UP;
	}


CLEAN_UP : ;

	return err;
}

int
SHP_read_bbox(FILE *fp, SF_BBOX_T *bbox)
{
	int	err = 0;

	memset(bbox, 0, sizeof(SF_BBOX_T));

	if(FIO_read_le_double(fp, &bbox->s_xmin)){
		LOG_ERROR("FIO_read_le_double failed for xmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fp, &bbox->s_ymin)){
		LOG_ERROR("FIO_read_le_double failed for ymin");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fp, &bbox->s_xmax)){
		LOG_ERROR("FIO_read_le_double failed for xmax");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fp, &bbox->s_ymax)){
		LOG_ERROR("FIO_read_le_double failed for ymax");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	// prevent info leak on err
	if(err)
		memset(bbox, 0, sizeof(SF_BBOX_T));

	return err;
}

int
SHP_read_ridx(FILE *fp, SF_RIDX_T *ridx)
{
	int	err = 0;

	if(FIO_read_be_int4(fp, &ridx->s_offset)){
		LOG_ERROR("FIO_read_be_int4 failed for s_offset");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_be_int4(fp, &ridx->s_length)){
		LOG_ERROR("FIO_read_be_int4 failed for s_length");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

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
	fprintf(fp, "\tfname   = %s\n", fhdr->s_fname ? fhdr->s_fname : "NULL");
	fprintf(fp, "\tmagic   = %d\n", fhdr->s_magic);
	fprintf(fp, "\tl_file  = %d\n", fhdr->sl_file);
	fprintf(fp, "\tversion = %d\n", fhdr->s_version);
	fprintf(fp, "\ttype    = %d\n", fhdr->s_type);
	fprintf(fp, "\tbbox     = {\n");
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

void
SHP_dump_ridx(FILE *fp, SF_RIDX_T *ridx)
{

	if(ridx == NULL){
		fprintf(fp, "ridx is NULL\n");
		return;
	}
	fprintf(fp, "%d\t%d\n", ridx->s_offset, ridx->s_length);
}

SF_SHAPE_T	*
SHP_read_shape(FILE *fp)
{
	SF_SHAPE_T	*shp = NULL;
	int	ival;
	double	dval;
	int	err = 0;

	shp = (SF_SHAPE_T *)calloc((size_t)1, sizeof(SF_SHAPE_T));
	if(shp == NULL){
		LOG_ERROR("can't allocate shp");
		err = 1;
		goto CLEAN_UP;
	}

	if(FIO_read_be_int4(fp, &ival)){
		LOG_ERROR("can't read record number");
		err = 1;
		goto CLEAN_UP;
	}
	shp->s_rnum = ival;
	if(FIO_read_be_int4(fp, &ival)){
		LOG_ERROR("can't read record length");
		err = 1;
		goto CLEAN_UP;
	}
	shp->s_length = ival;
	if(FIO_read_le_int4(fp, &ival)){
		LOG_ERROR("can't read record length");
		err = 1;
		goto CLEAN_UP;
	}
	shp->s_type = ival;

	// TODO: decice what to do base on shape type1

CLEAN_UP : ;

	if(err){
		SHP_delete_shape(shp);
		shp = NULL;
	}

	return shp;
}

void
SHP_delete_shape(SF_SHAPE_T *shp)
{

	if(shp == NULL)
		return;

	if(shp->s_parts != NULL)
		free(shp->s_parts);
	if(shp->s_ptypes != NULL)
		free(shp->s_ptypes);
	if(shp->s_points != NULL)
		free(shp->s_points);
	if(shp->s_zvals != NULL)
		free(shp->s_zvals);
	if(shp->s_mvals != NULL)
		free(shp->s_mvals);
	free(shp);
}

void
SHP_dump_shape(FILE *fp, SF_SHAPE_T *shp, int verbose)
{

	if(shp == NULL){
		fprintf(fp, "shp is NULL\n");
		return;
	}

	fprintf(fp, "shape = {\n");
	fprintf(fp, "\trnum = %d\n", shp->s_rnum);
	fprintf(fp, "\tlength = %d\n", shp->s_length);
	fprintf(fp, "\ttype   = %d\n", shp->s_type);
	fprintf(fp, "}\n");
}
