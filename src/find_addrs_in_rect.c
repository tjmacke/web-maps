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

#define	DEF_DIST	600

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-a",    0, AVK_REQ,  AVT_STR,  NULL, "Use -a AF file to use the sorted addresses in AF."},
	{"-d",    1, AVK_REQ,  AVT_REAL, ARGS_n2s(DEF_DIST),  "Use -d D to set the box size 2*D feet."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

static	int
find_addrs_in_box(const ADATA_T *, const ADDR_T *, double);

static	int
mk_box(const SF_POINT_T *, double, SF_POINT_T []);

static	void
fi_addr_bounds(double, const ADATA_T *, int *, int *);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*afname = NULL;
	double	dist = DEF_DIST;
	FILE	*fp = NULL;
	ADATA_T	*adata = NULL;
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lnum;
	ADDR_T	*ap = NULL;
	int	i, i_le, i_ge;
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

	a_val = TJM_get_flag_value(args, "-d", AVT_REAL);
	dist = a_val->av_value.v_double;

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

	for(lnum = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lnum++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_WARN("line %7d: empty line", lnum);
				continue;
			}
		}
		ap = AD_new_addr(line, lnum);
		if(ap == NULL){
			LOG_ERROR("line %7d: AD_new_addr failed", lnum);
			err = 1;
			goto CLEAN_UP;
		}
		find_addrs_in_box(adata, ap, dist);
		AD_delete_addr(ap);
		ap = NULL;
	}

CLEAN_UP : ;

	if(ap != NULL)
		AD_delete_addr(ap);

	if(adata != NULL)
		AD_delete_adata(adata);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}

// TODO: fix to remove linear search on lng
static	int
find_addrs_in_box(const ADATA_T *adp, const ADDR_T *ap, double size)
{
	SF_POINT_T	ctr;
	SF_POINT_T	box[4];
	int	bl_le, bl_ge;
	int	bh_le, bh_ge;
	int	pr_ap;
	int	i, i_first, i_last;
	const ADDR_T	*ap1;
	int	err = 0;

	ctr.s_x = ap->a_lng;
	ctr.s_y = ap->a_lat;
	mk_box(&ctr, size, box);

	// check the lowest lat of the box
	fi_addr_bounds(box[0].s_y, adp, &bl_le, &bl_ge);
	if(bl_ge == adp->an_atab){	// [t t] [b b]
		LOG_WARN("box is above the highest address");
		err = 1;
		goto CLEAN_UP;
	}

	// check the highest lat of the box
	fi_addr_bounds(box[1].s_y, adp, &bh_le, &bh_ge);
	if(bh_le == -1){		// [b b] [t t]
		LOG_WARN("box is below the lowest address");
		err = 1;
		goto CLEAN_UP;
	}

	// box and table overlap, deal w/ box.low < table.low and box.high > table.high
	i_first = bl_ge == -1 ? 0 : bl_ge;
	i_last = bh_le == adp->an_atab ? adp->an_atab - 1 : bh_le;
	for(pr_ap = 1, i = i_first; i <= i_last; i++){
		ap1 = adp->a_atab[i];
		if(ap1->a_lng < box[0].s_x || ap1->a_lng > box[3].s_x)
			continue;
		if(pr_ap){
			pr_ap = 0;
			printf("%s\n", ap->a_line);
		}
		printf("%s\n", ap1->a_line);
	}
	
CLEAN_UP : ;

	return err;
}

static	int
mk_box(const SF_POINT_T *ctr, double size, SF_POINT_T box[])
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

// TODO: fix this to explicit index, sort of like an rtree.
static	void
fi_addr_bounds(double lat, const ADATA_T *adp, int *i_le, int *i_ge)
{
	int	i, j, k;
	const ADDR_T	*ap;

	for(i = 0, j = adp->an_atab - 1; i <= j; ){
		k = (i + j) / 2;
		ap = adp->a_atab[k];
		if(ap->a_lat < lat)
			i = k + 1;
		else if(ap->a_lat > lat)
			j = k - 1;
		else{	// Found!  Use i == j to indicate the element was in the table
			i = j = k;
			break;
		}
	}
	*i_le = j;
	*i_ge = i;
}
