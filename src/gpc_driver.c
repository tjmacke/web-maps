#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gpc.h>

#include "log.h"
#include "args.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-h",    1, AVK_NONE, AVT_BOOL, "0",  "Use -h to expect/create \"hole flag\" in the inputs and output."},
	{"-op",   0, AVK_REQ,  AVT_STR,  "diff|int|xor|union", "Use -op O, O in {diff, int, xor, union} to set the type of clipping."},
	{"-clip", 0, AVK_REQ,  AVT_STR,  NULL, "Use -clip C to use the clip path in C."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

static	int
set_gpc_op(const char *, gpc_op *);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	int	hflag= 0;
	const char	*op_str = NULL;
	const char	*cfname = NULL;
	gpc_op	op;
	gpc_polygon	clip;
	gpc_polygon	subject;
	gpc_polygon	result;
	FILE	*cfp = NULL;
	FILE	*fp = NULL;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-h", AVT_BOOL);
	hflag = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-op", AVT_STR);
	op_str = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-clip", AVT_STR);
	cfname = a_val->av_value.v_str;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	if(set_gpc_op(op_str, &op)){
		LOG_ERROR("set_gpc_op failed");
		err = 1;
		goto CLEAN_UP;
	}

	if((cfp = fopen(cfname, "r")) == NULL){
		LOG_ERROR("can't read clip file %s", cfname);
		err = 1;
		goto CLEAN_UP;
	}

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read subject file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	gpc_read_polygon(cfp, hflag, &clip);
	gpc_read_polygon(fp, hflag, &subject);

	gpc_polygon_clip(op, &subject, &clip, &result);

	gpc_write_polygon(stdout, hflag, &result);

	gpc_free_polygon(&subject);
	gpc_free_polygon(&clip);
	gpc_free_polygon(&result);

CLEAN_UP : ;

	if(fp != NULL && fp != stdin)
		fclose(fp);

	if(cfp != NULL)
		fclose(cfp);

	TJM_free_args(args);

	exit(err);
}

static	int
set_gpc_op(const char *op_str, gpc_op *op)
{
	int	err = 0;

	if(op_str == NULL || *op_str == '\0'){
		LOG_ERROR("op_str is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}
	if(!strcmp(op_str, "diff"))
		*op = GPC_DIFF;
	else if(!strcmp(op_str, "int"))
		*op = GPC_INT;
	else if(!strcmp(op_str, "xor"))
		*op = GPC_XOR;
	else if(!strcmp(op_str, "union"))
		*op = GPC_UNION;
	else{
		LOG_ERROR("unknown clipping type: %s: must one-of {diff, int, xor, union}", op_str);
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	return err;
}
