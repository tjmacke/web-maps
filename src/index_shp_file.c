#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"
#include "index.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0", "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0", "Use -v to set the verbosity to 1; use -v=N to set it to N."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

static	int
index_shp_file(const char *);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	char	*fname = NULL;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 1, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	if(index_shp_file(args->a_files[0])){
		LOG_ERROR("index_shp_file failed for %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	TJM_free_args(args);

	exit(err);
}

static	int
index_shp_file(const char *fname)
{
	char	*shx_fname = NULL;
	SF_FHDR_T	*shx_fhdr = NULL;
	char	*ridx_fname = NULL;
	FILE	*ridx_fp = NULL;
	HDR_T	hdr;
	RIDX_T	ridx;
	SF_RIDX_T	sf_ridx;
	int	i;
	int	err = 0;

	shx_fname = SHP_make_sf_name(fname, "shx");
	if(shx_fname == NULL){
		LOG_ERROR("SHP_make_sf_name failed for \"shx\" file");
		err = 1;
		goto CLEAN_UP;
	}

LOG_DEBUG("fname      = %s", fname);
LOG_DEBUG("shx_fname  = %s", shx_fname);

	ridx_fname = SHP_make_sf_name(fname, "ridx");
	if(ridx_fname == NULL){
		LOG_ERROR("SHP_make_sf_name failed for \"ridx\" file");
		err = 1;
		goto CLEAN_UP;
	}
LOG_DEBUG("ridx_fname = %s", ridx_fname);

	shx_fhdr = SHP_open_file(shx_fname);
	if(shx_fhdr == NULL){
		LOG_ERROR("SHP_open_file failed for %s", shx_fname);
		err = 1;
		goto CLEAN_UP;
	}
	if((ridx_fp = fopen(ridx_fname, "w")) == NULL){
		LOG_ERROR("fopen failed for ridx file %s", ridx_fname);
		err = 1;
		goto CLEAN_UP;
	}

	hdr.h_count = (shx_fhdr->sl_file - SF_FHDR_SIZE) / SF_RIDX_SIZE;
	hdr.h_size = sizeof(RIDX_T);
	fwrite(&hdr, sizeof(HDR_T), (size_t)1, ridx_fp);
	for(i = 0; i < hdr.h_count; i++){
		if(SHP_read_ridx(shx_fhdr->s_fp, &sf_ridx)){
			LOG_ERROR("SHP_read_ridx failed for record number %d", i+1);
			err = 1;
			goto CLEAN_UP;
		}
		ridx.r_offset = SF_WORD_SIZE * sf_ridx.s_offset;
		ridx.r_length = SF_WORD_SIZE * sf_ridx.s_length;
		fwrite(&ridx, sizeof(RIDX_T), (size_t)1, ridx_fp);
	}

CLEAN_UP : ;

	if(ridx_fp != NULL)
		fclose(ridx_fp);
	if(ridx_fname != NULL)
		free(ridx_fname);

	if(shx_fhdr != NULL)
		SHP_close_file(shx_fhdr);
	if(shx_fname != NULL)
		free(shx_fname);

	return err;
}
