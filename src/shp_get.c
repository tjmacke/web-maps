#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "props.h"
#include "shape.h"
#include "s2g_input.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-what", 0, AVK_REQ,  AVT_STR,  "lines|bboxes", "Use -what O to select the objects to get."},
	{"-pf",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pf P to get shape titles; requires -pk K."},
	{"-pk",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pk K to set the primary key to K; requires -pf P."},
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
	const char	*pfname = NULL;
	const char	*pkey = NULL;
	int	all = 0;
	const char	*sf = NULL;
	const char	*fm_name = NULL;

	PROPERTIES_T	*props = NULL;
	const PROP_T	*pp;
	PROP_T	*def_pp = NULL;

	S2G_INPUT_T	*s2g = NULL;
	SF_SHAPE_T	*shp = NULL;

	FILE	*fp = NULL;
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lcnt;
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
	if(strcmp(what, "lines") && strcmp(what, "bboxes")){
		LOG_ERROR("unknown -what value %s, must lines or bboxes", what);
		TJM_print_help_msg(stderr, args);
		err = 1;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-pf", AVT_STR);
	pfname = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pk", AVT_STR);
	pkey = a_val->av_value.v_str;

	if(pfname != NULL && pkey == NULL){
		LOG_ERROR("-pf P requires -pk K.");
		TJM_print_help_msg(stderr, args);
		err = 1;
		goto CLEAN_UP;
	}else if(pfname == NULL && pkey != NULL){
		LOG_ERROR("-pk K requires -pf F.");
		TJM_print_help_msg(stderr, args);
		err = 1;
		goto CLEAN_UP;
	}

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

	s2g = S2G_new(verbose, sf, fm_name, all, fp);
	if(s2g == NULL){
		LOG_ERROR("S2G_new failed.");
		err = 1;
		goto CLEAN_UP;
	}

	if(pfname != NULL){
		props = PROPS_new_properties(pfname, pkey);
		if(props == NULL){
			LOG_ERROR("PROPS_new_properties failed.");
			err = 1;
			goto CLEAN_UP;
		}
		if(PROPS_read_properties(props, sf != NULL)){
			LOG_ERROR("PROPS_read_properties failed.");
			err = 1;
			goto CLEAN_UP;
		}
		if(verbose)
			PROPS_dump_properties(stderr, props);
	}else{
		// props is needed to get the title or name for each selected object
		props = PROPS_make_default_ptab("title");
		if(props == NULL){
			LOG_ERROR("PROPS_make_default_ptab failed.");
			err = 1;
			goto CLEAN_UP;
		}
	}

	for(s2g->sl_fme = NULL, lcnt = 0; (l_line = S2G_getline(s2g, &line, &s_line)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_WARN("line: %7d: empty line.", lcnt);
				continue;
			}
		}
		shp = S2G_get_shape(s2g, line);
		if(shp == NULL){
			LOG_ERROR("line %7d: S2G_get_shape failed for key: %s.", lcnt, line); 
			err = 1;
			goto CLEAN_UP;
		}
		if(verbose)
			SHP_dump_shape(stderr, shp, verbose);
		// do the work
		SHP_delete_shape(shp);
		shp = NULL;
		// deal with property
	}

CLEAN_UP : ;

	if(shp != NULL)
		SHP_delete_shape(shp);

	if(line != NULL)
		free(line);

	if(props != NULL)
		PROPS_delete_properties(props);

	if(s2g != NULL)
		S2G_delete(s2g);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}
