#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "adata.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-a",    0, AVK_REQ,  AVT_STR,  NULL, "Use -a AF file to use the sorted addresses in AF."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*afname = NULL;
	FILE	*fp = NULL;
	ADATA_T	*adata = NULL;
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

CLEAN_UP : ;

	if(adata != NULL)
		AD_delete_adata(adata);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}
