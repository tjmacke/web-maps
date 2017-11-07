#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "dbase.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-t",    0, AVK_REQ,  AVT_STR,  NULL, "Use -t T to put the dbase data into a sqlite table named T."},
	{"-pk",   0, AVK_REQ,  AVT_STR,  NULL, "Use -pk F to use field F as the primary key; use -pk +F to add field F as the primary key."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*tbl = NULL;;
	const char	*pk = NULL;
	FILE	*fp = NULL;
	DBF_META_T	*dbm = NULL;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-t", AVT_STR);
	tbl = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pk", AVT_STR);
	pk = a_val->av_value.v_str;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read dbase file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	dbm = DBF_new_dbf_meta(args->an_files == 0 ? "--stdin--" : args->a_files[0]);
	if(dbm == NULL){
		LOG_ERROR("DBF_new_dbf_meta failed for %s", args->an_files == 0 ? "--stdin--" : args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}
	if(DBF_read_dbf_meta(fp, dbm)){
		LOG_ERROR("DBF_read_dbf_meta failed for %s", args->an_files == 0 ? "--stdin--" : args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}
	// make the create table T command
	// make the insert into T commands

CLEAN_UP : ;

	if(dbm != NULL)
		DBF_delete_dbf_meta(dbm);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}
