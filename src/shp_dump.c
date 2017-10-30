#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"

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
	int	ftype = SFT_UNKNOWN;
	FILE	*fp = NULL;
	SF_FHDR_T	fhdr;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 1, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	/*
	   2. use the right code to dump the file
	*/

	ftype = SHP_get_file_type(args->a_files[0]);
	if(ftype == SFT_UNKNOWN){
		LOG_ERROR("SHP_get_file_type failed for %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read input file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	switch(ftype){
	case SFT_SHP :
		if(SHP_read_fhdr(fp, &fhdr)){
			LOG_ERROR("SHP_read_fhdr failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
		SHP_dump_fhdr(stdout, &fhdr);
		break;
	case SFT_SHX :
		break;
	case SFT_DBF :
		break;
	case SFT_PRJ :
		break;
	default :
		LOG_ERROR("unkonwn file type %d", ftype);
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(fp != NULL)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}
