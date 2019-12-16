#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-what", 0, AVK_REQ,  AVT_STR,  "lines|bboxes", "Use -what O to select the objects to get."},
	{"-all",  1, AVK_NONE, AVT_BOOL, "0",  "Use -all to get objects from all elements in the src; any input file is ignored."},
	{"",      1, AVK_MSG,  AVT_BOOL, "0",  "The data sources: choose one of -sf S or -fmap F."},
	{"-sf",   1, AVK_REQ,  AVT_STR,  NULL, "Use -sf S to get objects from some or all of the shapes in S.shp, S.shx."},
	{"-fmap", 1, AVK_REQ,  AVT_STR,  NULL, "Use -fmap F to get objects from some or all othe shapes in file map F."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*what = NULL;
	int	all = 0;
	const char	*sf = NULL;
	const char	*fm_name = NULL;
	FILE	*fp = NULL;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-what", AVT_STR);
	what = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-all", AVT_BOOL);
	all = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-sf", AVT_STR);
	sf = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-fmap", AVT_STR);
	fm_name = a_val->av_value.v_str;

	if(sf == NULL && fm_name == NULL){
		LOG_ERROR("one of -sf S or -fmap F must be set.");
		err = 1;
		goto CLEAN_UP;
	}else if(sf != NULL && fm_name != NULL){
		LOG_ERROR("only one of -sf S or -fmap F cat be set.");
		err = 1;
		goto CLEAN_UP;
	}

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	if(!all){	// -all means ignore any file arg
		if(args->an_files == 0)
			fp = stdin;
		else if((fp = fopen(args->a_files[0], "r")) == NULL){
			LOG_ERROR("can't read rnum file %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;

	TJM_free_args(args);

	exit(err);
}
