#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "args.h"
#include "props.h"
#include "shape.h"
#include "s2g_input.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-sc",   1, AVK_REQ,  AVT_STR,  NULL, "Use -sc SC to add the json in SC to as \"scaleConfig\" to object holding the geojson."},
	{"-pf",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pf P to to add properties to the geojson."},
	{"-pk",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pk K to set the primary key to K."},
	{"-fmt",  1, AVK_REQ,  AVT_STR,  "wrap*|plain|list",  "Use -fmt F, F in {wrap, plain, list} to control the format of the json output."},
	{"-all",  1, AVK_NONE, AVT_BOOL, "0",  "Use -all to convert all elements in the src to geojson; any input file is ignored."},
	{"",      1, AVK_MSG,  AVT_BOOL, "0",  "The data sources: choose one of: -sf S or -fmap F"},
	{"-sf",   1, AVK_REQ,  AVT_STR,  NULL, "Use -sf S to convert some or all of the shapes in S.shp, S.shx to geojson."},
	{"-fmap", 1, AVK_REQ,  AVT_STR,  NULL, "Use -fmap F to convert some or all of the shapes in file map F to geojson."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*scfname = NULL;
	const char	*pfname = NULL;
	const char	*pkey = NULL;
	const char	*fmt = NULL;
	int	all = 0;
	const char	*sf = NULL;
	const char	*fm_name = NULL;

	PROPERTIES_T	*props = NULL;
	const PROP_T	*pp;
	PROP_T	*def_pp = NULL;
	char	*js_props = NULL;

	S2G_INPUT_T	*s2g = NULL;
	SF_SHAPE_T	*shp = NULL;

	FILE	*fp = NULL;
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lcnt;
	int	a_prlg, first;

	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-sc", AVT_STR);
	scfname = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pf", AVT_STR);
	pfname = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pk", AVT_STR);
	pkey = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-fmt", AVT_STR);
	fmt = a_val->av_value.v_str;
	if(strcmp(fmt, "wrap")){
		if(scfname != NULL){
			LOG_ERROR("-sc SC option requires -fmt wrap");
			err = 1;
			goto CLEAN_UP;
		}
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
		LOG_ERROR("only one of -sf S or -fmap F can be set.");
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
		LOG_ERROR("S2G_new failed");
		err = 1;
		goto CLEAN_UP;
	}

	if(pfname != NULL){
		props = PROPS_new_properties(pfname, pkey);
		if(props == NULL){
			LOG_ERROR("PROPS_new_properties failed");
			err = 1;
			goto CLEAN_UP;
		}
		if(PROPS_read_properties(props, sf != NULL)){
			LOG_ERROR("PROPS_read_properties failed");
			err = 1;
			goto CLEAN_UP;
		}
		if(verbose)
			PROPS_dump_properties(stderr, props);
	}else{
		// props is needed to convert properties to json.
		// in this case, the table contains only a header w/key field = "title"
		// The prop will be created directly as ("title", name/number of current shape)
		props = PROPS_make_default_ptab("title");
		if(props == NULL){
			LOG_ERROR("PROPS_make_default_ptab failed");
			err = 1;
			goto CLEAN_UP;
		}
	}

	for(s2g->sl_fme = NULL, a_prlg = first = 1, lcnt = 0;
		(l_line = S2G_getline(s2g, &line, &s_line)) > 0; 
		/*(l_line = getline(&line, &s_line, fp)) > 0;*/
	){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_WARN("line %7d: empty line", lcnt);
				continue;
			}
		}
		shp = S2G_get_shape(s2g, line);
		if(shp == NULL){
			LOG_ERROR("line %7d: S2G_get_shape failed for key: %s\n", lcnt, line);
			err = 1;
			goto CLEAN_UP;
		}
		if(verbose)
			SHP_dump_shape(stderr, shp, verbose);
		if(a_prlg){
			a_prlg = 0;
			if(SHP_write_geojson_prolog(stdout, fmt, scfname)){
				LOG_ERROR("SHP_write_geojson_prolog failed");
				err = 1;
				goto CLEAN_UP;
			}
		}
		def_pp = NULL;
		js_props = NULL;
		if(s2g->s_sf != NULL){
			pp = PROPS_find_props_with_int_key(props, shp->s_rnum);
			if(pp == NULL){
				char	work[20];

				sprintf(work, GJ_DEFAULT_TITLE_FMT_D, shp->s_rnum);
				LOG_WARN("no properties for rnum = %d, using (\"title\", \"%s\")", shp->s_rnum, work);
				def_pp = PROPS_new_prop("title", 1, work);
				if(def_pp == NULL){
					LOG_ERROR("PROPS_new_prop failed for %s", line);
					err = 1;
					goto CLEAN_UP;
				}
				pp = def_pp;
			}
		}else{
			pp = PROPS_find_props_with_str_key(props, line);
			if(pp == NULL){
				LOG_WARN("no properties for key = %s, using (\"title\", \"%s\")", line, line);
				def_pp = PROPS_new_prop("title", 0, line);
				if(def_pp == NULL){
					LOG_ERROR("PROPS_new_prop failed for %s", line);
					err = 1;
					goto CLEAN_UP;
				}
				pp = def_pp;
			}
		}
		js_props = PROPS_to_json_object(props, pp);
		if(js_props == NULL){
			LOG_ERROR("PROPS_to_json_object failed");
			err = 1;
			goto CLEAN_UP;
		}
		SHP_write_geojson(stdout, shp, first, js_props);
		SHP_delete_shape(shp);
		shp = NULL;
		if(def_pp != NULL){
			PROPS_delete_prop(def_pp);
			def_pp = NULL;
		}
		free(js_props);
		js_props = NULL;
		first = 0;
	}
	if(!a_prlg)
		SHP_write_geojson_trailer(stdout, fmt);

CLEAN_UP : ;

	if(def_pp != NULL)
		PROPS_delete_prop(def_pp);

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

static	char	*
PROPS_get_as_json_object(const PROPERTIES_T *props, int use_rnum, int rnum, const char *str)
{
	const PROP_T	*pp;
	PROP_T	*def_pp = NULL;
	char	*p_objstr = NULL;
	int	err = 0;

	if(use_rnum){
		pp = PROPS_find_props_with_int_key(props, rnum);
		if(pp == NULL){
			LOG_WARN("no properties for rnum = %d", rnum);
			err = 1;
		}else{
			p_objstr = PROPS_to_json_object(props, pp);
			if(p_objstr == NULL){
				LOG_ERROR("PROPS_to_json_object failed");
				err = 1;
				goto CLEAN_UP;
			}
		}
	}else{
		pp = PROPS_find_props_with_str_key(props, str);
		if(pp == NULL){
			def_pp = PROPS_new_prop("title", 0, str);
			if(def_pp == NULL){
				LOG_ERROR("PROPS_new_prop failed for %s", str);
				err = 1;
				goto CLEAN_UP;
			}
			p_objstr = PROPS_to_json_object(props, def_pp);
			PROPS_delete_prop(def_pp);
			def_pp = NULL;
		}else
			p_objstr = PROPS_to_json_object(props, pp);
		if(p_objstr == NULL){
			LOG_ERROR("PROPS_to_json_object failed");
			err = 1;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;

	if(err){
		if(p_objstr != NULL){
			free(p_objstr);
			p_objstr = NULL;
		}
	}

	return p_objstr;
}
