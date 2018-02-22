#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "log.h"
#include "args.h"
#include "shape.h"
#include "adata.h"

// BEGIN: rectangle about a point stuff
#define	PI	3.14159265
#define	D2R	(2.0*PI/360.0)
#define	R2D	(360.0/(2.0*PI))
#define	FPM	3.2808		// feet/meter

#define	R_MAJOR	6378137.0	// in meters
#define	R_MINOR	6356752.3	// in meters
#define	R_AVG_FT	(FPM*0.5*(R_MAJOR+R_MINOR))
#define	DPF_EQ	(1.0/(2.0*PI*R_AVG_FT/360.0))
// END: rectangle about a point stuff

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-a",    0, AVK_REQ,  AVT_STR,  NULL, "Use -a AF file to use the sorted addresses in AF."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

static	int
mk_rect(const SF_POINT_T *, double, SF_POINT_T []);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*afname = NULL;
	FILE	*fp = NULL;
	ADATA_T	*adata = NULL;
	SF_POINT_T	ctr;
	SF_POINT_T	box[4];
	int	i;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-a", AVT_STR);
	afname = a_val->av_value.v_str;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read input file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	// read the addresses
	adata = AD_new_adata(afname, 5000);
	if(adata == NULL){
		LOG_ERROR("AD_new_adata failed");
		err = 1;
		goto CLEAN_UP;
	}
	if(AD_read_adata(adata, verbose)){
		LOG_ERROR("AD_read_adata failed");
		err = 1;
		goto CLEAN_UP;
	}
	if(verbose)
		AD_dump_adata(stderr, adata, verbose);

	ctr.s_x = -122.320414;
	ctr.s_y = 47.615923000000002;
	printf("%s\t%s\t%s\t%.15e\t%.15e\t%s\n", ".", "ctr", ".", ctr.s_x, ctr.s_y, ".");
	mk_rect(&ctr, 600.0, box);
	for(i = 0; i < 4; i++)
		printf("%s\tbox[%d]\t%s\t%.15e\t%.15e\t%s\n", ".", i, ".", box[i].s_x, box[i].s_y, ".");

CLEAN_UP : ;

	if(adata != NULL)
		AD_delete_adata(adata);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}

static	int
mk_rect(const SF_POINT_T *ctr, double size, SF_POINT_T box[])
{
	double	dpf_lat, lng_adj, lat_adj;
	int	err = 0;

	dpf_lat = DPF_EQ / cos(D2R * ctr->s_y);
	lat_adj = size * DPF_EQ;
	lng_adj = size * dpf_lat;

	// lower left
	box[0].s_x = ctr->s_x - lng_adj;
	box[0].s_y = ctr->s_y - lat_adj;

	// upper left
	box[1].s_x = ctr->s_x -lng_adj;
	box[1].s_y = ctr->s_y + lat_adj; 

	// upper right
	box[2].s_x = ctr->s_x + lng_adj;
	box[2].s_y = ctr->s_y + lat_adj;

	// lower right
	box[3].s_x = ctr->s_x + lng_adj;
	box[3].s_y = ctr->s_y - lat_adj; 

	return err;
}
