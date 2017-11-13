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

SF_FHDR_T	*
SHP_open_file(const char *fname)
{
	SF_FHDR_T	*fhdr = NULL;
	int	ival, i, c;
	double	dval;
	int	err = 0;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	fhdr = SHP_new_fhdr(fname);
	if(fhdr == NULL){
		LOG_ERROR("SHP_new_fhdr failed for %s", fname);
		err = 1;
		goto CLEAN_UP;
	}
	if((fhdr->s_fp = fopen(fhdr->s_fname, "r")) == NULL){
		LOG_ERROR("can't read file %s", fhdr->s_fname);
		err = 1;
		goto CLEAN_UP;
	}

	// values 1-7 are big endian
	// read the magic number
	if(FIO_read_be_int4(fhdr->s_fp, &ival)){
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
		if(FIO_read_be_int4(fhdr->s_fp, &ival)){
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
	if(FIO_read_be_int4(fhdr->s_fp, &ival)){
		LOG_ERROR("FIO_read_be_int4 failed for file length");
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->sl_file = ival;	// 2 byte words!

	// the remaining values are little endian
	// get the version
	if(FIO_read_le_int4(fhdr->s_fp, &ival)){
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
	if(FIO_read_le_int4(fhdr->s_fp, &ival)){
		LOG_ERROR("FIO_read_le_int4 failed for version");
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->s_type = ival;

	if(SHP_read_bbox(fhdr->s_fp, &fhdr->s_bbox)){
		LOG_ERROR("SHP_get_bbox failed");
		err = 1;
		goto CLEAN_UP;
	}

	// get the z range
	if(FIO_read_le_double(fhdr->s_fp, &fhdr->s_zmin)){
		LOG_ERROR("FIO_read_le_double failed for zmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fhdr->s_fp, &fhdr->s_zmax)){
		LOG_ERROR("FIO_read_le_double failed for zmax");
		err = 1;
		goto CLEAN_UP;
	}

	// get the m range
	if(FIO_read_le_double(fhdr->s_fp, &fhdr->s_mmin)){
		LOG_ERROR("FIO_read_le_double failed for mmin");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fhdr->s_fp, &fhdr->s_mmax)){
		LOG_ERROR("FIO_read_le_double failed for mmax");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(err){
		SHP_close_file(fhdr);
		fhdr = NULL;
	}

	return fhdr;
}

void
SHP_close_file(SF_FHDR_T *fhdr)
{

	if(fhdr == NULL)
		return;

	if(fhdr->s_fp != NULL)
		fclose(fhdr->s_fp);
	if(fhdr->s_fname != NULL)
		free(fhdr->s_fname);
	free(fhdr);
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

void
SHP_dump_bbox(FILE *fp, const SF_BBOX_T *bbox, const char *indent)
{

	fprintf(fp, "%sbbox     = {\n", indent != NULL ? indent : "");
	fprintf(fp, "%s\txmin   = %.15e\n", indent != NULL ? indent : "", bbox->s_xmin);
	fprintf(fp, "%s\tymin   = %.15e\n", indent != NULL ? indent : "", bbox->s_ymin);
	fprintf(fp, "%s\txmax   = %.15e\n", indent != NULL ? indent : "", bbox->s_xmax);
	fprintf(fp, "%s\tymax   = %.15e\n", indent != NULL ? indent : "", bbox->s_ymax);
	fprintf(fp, "%s}\n", indent != NULL ? indent : "");
}

int
SHP_read_point(FILE *fp, SF_POINT_T *pt)
{
	int	err = 0;

	memset(pt, 0, sizeof(SF_POINT_T));

	if(FIO_read_le_double(fp, &pt->s_x)){
		LOG_ERROR("FIO_read_le_double failed for x");
		err = 1;
		goto CLEAN_UP;
	}
	if(FIO_read_le_double(fp, &pt->s_y)){
		LOG_ERROR("FIO_read_le_double failed for y");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	// prevent info leak on error
	if(err)
		memset(pt, 0, sizeof(SF_POINT_T));

	return err;
}

void
SHP_dump_point(FILE *fp, const SF_POINT_T *pt, const char *indent, int u_blk)
{

	if(!u_blk)
		fprintf(fp, "%s%.15e\t%.15e\n", indent ? indent : "", pt->s_x, pt->s_y);
	else{
		fprintf(fp, "%spoint = {\n", indent ? indent : "");
		fprintf(fp, "%s\tx = %.15e\n", indent ? indent : "", pt->s_x);
		fprintf(fp, "%s\ty = %.15e\n", indent ? indent : "", pt->s_y);
		fprintf(fp, "%s}\n", indent ? indent : "");
	}
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
	SHP_dump_bbox(fp, &fhdr->s_bbox, "\t");
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
	int	n_words;
	int	ival;
	double	dval;
	int	err = 0;

	shp = (SF_SHAPE_T *)calloc((size_t)1, sizeof(SF_SHAPE_T));
	if(shp == NULL){
		LOG_ERROR("can't allocate shp");
		err = 1;
		goto CLEAN_UP;
	}

	// read the record header
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

	n_words = 0;
	if(FIO_read_le_int4(fp, &ival)){
		LOG_ERROR("can't read record length");
		err = 1;
		goto CLEAN_UP;
	}
	shp->s_type = ival;
	n_words += 2;

	if(shp->s_type != ST_NULL && !ST_IS_POINT_TYPE(shp->s_type)){
		if(SHP_read_bbox(fp, &shp->s_bbox)){
			LOG_ERROR("SHP_read_bbox failed");
			err = 1;
			goto CLEAN_UP;
		}
		n_words += 16;
	}
	if(ST_IS_PLINE_TYPE(shp->s_type) || ST_IS_PGON_TYPE(shp->s_type) || shp->s_type == ST_MULTIPATCH){
		if(FIO_read_le_int4(fp, &ival)){
			LOG_ERROR("can't read nparts");
			err = 1;
			goto CLEAN_UP;
		}
		shp->sn_parts = ival;
		n_words += 2;
	}

	if(shp->s_type != ST_NULL && !ST_IS_POINT_TYPE(shp->s_type)){
		if(FIO_read_le_int4(fp, &ival)){
			LOG_ERROR("can't read nparts");
			err = 1;
			goto CLEAN_UP;
		}
		shp->sn_points = ival;
		n_words += 2;
	}else
		shp->sn_points = 1;

	if(ST_IS_PLINE_TYPE(shp->s_type) || ST_IS_PGON_TYPE(shp->s_type) || shp->s_type == ST_MULTIPATCH){
		int	i;

		shp->s_parts = (int *)malloc(shp->sn_parts * sizeof(int));
		if(shp->s_parts == NULL){
			LOG_ERROR("can't allocate s_parts");
			err = 1;
			goto CLEAN_UP;
		}
		for(i = 0; i < shp->sn_parts; i++){
			if(FIO_read_le_int4(fp, &ival)){
				LOG_ERROR("can't read parts[%d]", i+1);
				err = 1;
				goto CLEAN_UP;
			}
			shp->s_parts[i] = ival;
			n_words += 2;
		}
	}

	if(shp->s_type == ST_MULTIPATCH){
		// read the ptypes
		int	i;

		shp->s_ptypes = (int *)malloc(shp->sn_parts * sizeof(int));
		if(shp->s_ptypes == NULL){
			LOG_ERROR("can't allocate s_ptypes");
			err = 1;
			goto CLEAN_UP;
		}
		for(i = 0; i < shp->sn_parts; i++){
			if(FIO_read_le_int4(fp, &ival)){
				LOG_ERROR("can't read ptypes[%d]", i+1);
				err = 1;
				goto CLEAN_UP;
			}
			shp->s_ptypes[i] = ival;
			n_words += 2;
		}
	}

	// read the points -- point types get an array w/1 elt
	if(shp->s_type != ST_NULL){
		int	i;

		shp->s_points = (SF_POINT_T *)malloc(shp->sn_points * sizeof(SF_POINT_T));
		if(shp->s_points == NULL){
			LOG_ERROR("can't allocate s_points");
			err = 1;
			goto CLEAN_UP;
		}
		for(i = 0; i < shp->sn_points; i++){
			if(SHP_read_point(fp, &shp->s_points[i])){
				LOG_ERROR("can't read points[%d]", i+1);
				err = 1;
				goto CLEAN_UP;
			}
			n_words += 8;
		}
	}

	// for types w/Z except point types, read the Z range
	// read the Z vals -- point types get an array w/1 elt.
	if(ST_HAS_ZDATA(shp->s_type)){
		double	dval;
		int	i;
		
		if(shp->s_type != ST_POINT_Z){
			if(FIO_read_le_double(fp, &dval)){
				LOG_ERROR("FIO_read_le_double failed for zmin");
				err = 1;
				goto CLEAN_UP;
			}
			shp->s_zmin = dval;
			n_words += 4;
			if(FIO_read_le_double(fp, &dval)){
				LOG_ERROR("FIO_read_le_double failed for zmax");
				err = 1;
				goto CLEAN_UP;
			}
			shp->s_zmax = dval;
			n_words += 4;
		}
		shp->s_zvals = (double *)malloc(shp->sn_points * sizeof(double));
		if(shp->s_zvals == NULL){
			LOG_ERROR("can't allocate zvals");
			err = 1;
			goto CLEAN_UP;
		}
		for(i = 0; i < shp->sn_points; i++){
			if(FIO_read_le_double(fp, &shp->s_zvals[i])){
				LOG_ERROR("FIO_read_le_double failed for zvals[%d]", i+1);
				err = 1;
				goto CLEAN_UP;
			}
			n_words += 4;
		}
	}

	// M data is optional for all but ST_POINT_Z & ST_POINT_M
	if(ST_HAS_MDATA(shp->s_type) && n_words < shp->s_length){
		int	i;

		if(shp->s_type != ST_POINT_Z && shp->s_type != ST_POINT_M){
			if(FIO_read_le_double(fp, &dval)){
				LOG_ERROR("FIO_read_le_double failed for mmin");
				err = 1;
				goto CLEAN_UP;
			}
			shp->s_mmin = dval;
			n_words += 4;
			if(FIO_read_le_double(fp, &dval)){
				LOG_ERROR("FIO_read_le_double failed for zmax");
				err = 1;
				goto CLEAN_UP;
			}
			shp->s_mmax = dval;
			n_words += 4;
		}
		shp->s_mvals = (double *)malloc(shp->sn_points * sizeof(double));
		if(shp->s_mvals == NULL){
			LOG_ERROR("can't allocate mvals");
			err = 1;
			goto CLEAN_UP;
		}
		for(i = 0; i < shp->sn_points; i++){
			if(FIO_read_le_double(fp, &shp->s_mvals[i])){
				LOG_ERROR("FIO_read_le_double failed for mvals[%d]", i+1);
				err = 1;
				goto CLEAN_UP;
			}
			n_words += 4;
		}
	}

	if(n_words != shp->s_length){
		LOG_ERROR("bad record length %d, expect %d", n_words, shp->s_length);
		err = 1;
		goto CLEAN_UP;
	}

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
	if(shp->s_type != ST_NULL && !ST_IS_POINT_TYPE(shp->s_type))
		SHP_dump_bbox(fp, &shp->s_bbox, "\t");
	if(ST_IS_PLINE_TYPE(shp->s_type) || ST_IS_PGON_TYPE(shp->s_type) || shp->s_type == ST_MULTIPATCH)
		fprintf(fp, "\tnparts = %d\n", shp->sn_parts);
	if(shp->s_type != ST_NULL)	// everything else has >= 1 point
		fprintf(fp, "\tnpoints = %d\n", shp->sn_points);
	if(ST_IS_PLINE_TYPE(shp->s_type) || ST_IS_PGON_TYPE(shp->s_type) || shp->s_type == ST_MULTIPATCH){
		int	i;

		fprintf(fp, "\tparts = {\n");
		for(i = 0; i < shp->sn_parts; i++)
			fprintf(fp, "\t\t%d\n", shp->s_parts[i]);
		fprintf(fp, "\t}\n");
	}
	if(shp->s_type == ST_MULTIPATCH){
		int	i;

		fprintf(fp, "\tptypes = {\n");
		for(i = 0; i < shp->sn_parts; i++)
			fprintf(fp, "\t\t%d\n", shp->s_parts[i]);
		fprintf(fp, "\t}\n");
	}
	if(shp->s_type != ST_NULL){
		int	i, p, pf, pl;

		fprintf(fp, "\tpoints = {\n");
		if(ST_HAS_PARTS(shp->s_type)){
			for(p = 0; p < shp->sn_parts; p++){
				pf = shp->s_parts[p];
				pl = (p == shp->sn_parts - 1) ? shp->sn_points : shp->s_parts[p+1];
				fprintf(fp, "\t\tpart = %d {\n", p+1);
				for(i = pf; i < pl; i++)
					SHP_dump_point(fp, &shp->s_points[i], "\t\t\t", 0);
				fprintf(fp, "\t\t}\n");
			}
		}else{
			for(i = 0; i < shp->sn_points; i++)
				SHP_dump_point(fp, &shp->s_points[i], "\t\t", 0);
		}
		fprintf(fp, "\t}\n");
	}
	if(ST_HAS_ZDATA(shp->s_type)){
		if(shp->s_type != ST_POINT_Z){
			int	i, p, pf, pl;

			fprintf(fp, "\tzrange = {\n");
			fprintf(fp, "\t\tmin = %.15e\n", shp->s_zmin);
			fprintf(fp, "\t\tmax = %.15e\n", shp->s_zmax);
			fprintf(fp, "\t}\n");
			fprintf(fp, "\tzvals = {\n");
			if(ST_HAS_PARTS(shp->s_type)){
				for(p = 0; p < shp->sn_parts; p++){
					pf = shp->s_parts[p];
					pl = (p == shp->sn_parts - 1) ? shp->sn_parts : shp->s_parts[p+1];
					fprintf(fp, "\t\tpart = %d {\n", p+1);
					for(i = pf; i < pl; i++)
						fprintf(fp, "\t\t\t%.15e\n", shp->s_zvals[i]);
					fprintf(fp, "\t\t}\n");
				}
			}else{
				for(i = 0; i < shp->sn_points; i++)
					fprintf(fp, "\t\t%.15e\n", shp->s_zvals[i]);
			}
			fprintf(fp, "\t}\n");
		}else
			fprintf(fp, "\tzval = %.15e\n", shp->s_zvals[0]);
	}
	if(ST_HAS_MDATA(shp->s_type) && shp->s_mvals != NULL){
		if(shp->s_type != ST_POINT_Z && shp->s_type != ST_POINT_M){
			int	i, p, pf, pl;

			fprintf(fp, "\tmrange = {\n");
			fprintf(fp, "\t\tmin = %.15e\n", shp->s_mmin);
			fprintf(fp, "\t\tmax = %.15e\n", shp->s_mmax);
			fprintf(fp, "\t}\n");
			fprintf(fp, "\tmvals = {\n");
			if(ST_HAS_PARTS(shp->s_type)){
				for(p = 0; p < shp->sn_parts; p++){
					pf = shp->s_parts[p];
					pl = (p == shp->sn_parts - 1) ? shp->sn_parts : shp->s_parts[p+1];
					fprintf(fp, "\t\tpart = %d {\n", p+1);
					for(i = pf; i < pl; i++)
						fprintf(fp, "\t\t\t%.15e\n", shp->s_mvals[i]);
					fprintf(fp, "\t\t}\n");
				}
			}else{
				for(i = 0; i < shp->sn_points; i++)
					fprintf(fp, "\t\t%.15e\n", shp->s_mvals[i]);
			}
			fprintf(fp, "\t}\n");
		}else
			fprintf(fp, "\tmval = %.15e\n", shp->s_mvals[0]);

	}
	fprintf(fp, "}\n");
}
