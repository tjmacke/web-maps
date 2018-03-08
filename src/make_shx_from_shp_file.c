#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"
#include "file_io.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0", "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0", "Use -v to set the verbosity to 1; use -v=N to set it to N."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	char	*shp_fname = NULL;
	char	*shx_fname = NULL;
	SF_FHDR_T	*shp_fhdr = NULL;
	FILE	*shx_fp = NULL;
	SF_SHAPE_T	*shp = NULL;
	SF_RIDX_T	ridx;
	int	n_recs, c, l_file;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 1, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	shp_fname = SHP_make_sf_name(args->a_files[0], "shp");
	if(shp_fname == NULL){
		LOG_ERROR("SHP_make_sf_name failed to make shp file name");
		err = 1;
		goto CLEAN_UP;
	}

	shx_fname = SHP_make_sf_name(args->a_files[0], "shx");
	if(shp_fname == NULL){
		LOG_ERROR("SHP_make_sf_name failed to make shx file name");
		err = 1;
		goto CLEAN_UP;
	}

	shp_fhdr = SHP_open_file(shp_fname);
	if(shp_fhdr == NULL){
		LOG_ERROR("SHP_open_file failed for %s", shx_fname);
		err = 1;
		goto CLEAN_UP;
	}

	if((shx_fp = fopen(shx_fname, "w")) == NULL){
		LOG_ERROR("can't write shx file %s", shx_fname);
		err = 1;
		goto CLEAN_UP;
	}
	SHP_write_fhdr(shx_fp, shp_fhdr);

	for(n_recs = 0; (c = getc(shp_fhdr->s_fp)) != EOF; ){
		ungetc(c, shp_fhdr->s_fp);
		ridx.s_offset = ftell(shp_fhdr->s_fp)/SF_WORD_SIZE;
		shp = SHP_read_shape(shp_fhdr->s_fp);
		if(shp == NULL){
			LOG_ERROR("SHP_read_shape failed for record %d", n_recs+1);
			err = 1;
			goto CLEAN_UP;
		}
		SHP_delete_shape(shp);
		shp = NULL;
		n_recs++;
		ridx.s_length = ftell(shp_fhdr->s_fp)/SF_WORD_SIZE - SF_RIDX_SIZE - ridx.s_offset;
		SHP_write_ridx(shx_fp, &ridx);
	}
	rewind(shx_fp);
	fseek(shx_fp, 24, SEEK_SET);
	l_file = SF_FHDR_SIZE + n_recs * SF_RIDX_SIZE;
	FIO_write_be_int4(shx_fp, l_file);

CLEAN_UP : ;

	if(shp != NULL)
		SHP_delete_shape(shp);

	if(shx_fp != NULL)
		fclose(shx_fp);

	if(shp_fhdr != NULL)
		SHP_delete_fhdr(shp_fhdr);

	if(shx_fname != NULL)
		free(shx_fname);

	if(shp_fname != NULL)
		free(shp_fname);

	TJM_free_args(args);

	exit(err);
}
