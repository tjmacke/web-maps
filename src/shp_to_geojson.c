#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"
#include "props.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-pf",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pf P to to add properties to the geojson."},
	{"-pk",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pk K to set the primary key to K."},
	{"-fmt",  1, AVK_REQ,  AVT_STR,  "wrap*|plain|list",  "Use -fmt F, F in {wrap, plain, list} to control the format of the json output."},
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
	const char	*pfname = NULL;
	const char	*pkey = NULL;
	const char	*fmt = NULL;
	const char	*sf = NULL;
	const char	*fm_name = NULL;
	PROPERTIES_T	*props = NULL;
	const PROP_T	*pp;
	char	*p_value;
	FILE	*fp = NULL;
	char	*shp_fname = NULL;
	SF_FHDR_T	*shp_fhdr = NULL;
	char	*shx_fname = NULL;
	int	n_recs = 0;
	SF_RIDX_T	*ridx = NULL;
	SF_RIDX_T	*rip;
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lcnt;
	int	i, rnum;
	int	a_prlg, first;
	SF_SHAPE_T	*shp = NULL;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-pf", AVT_STR);
	pfname = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pk", AVT_STR);
	pkey = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-fmt", AVT_STR);
	fmt = a_val->av_value.v_str;

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

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read rnum file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	if(sf != NULL){	// data in a single shp/shx pair
		// make the names from the value -sf
		shp_fname = SHP_make_sf_name(sf, "shp");
		if(shp_fname == NULL){
			LOG_ERROR("SHP_make_sf_name failed for \"shp\" file");
			err = 1;
			goto CLEAN_UP;
		}
		shx_fname = SHP_make_sf_name(sf, "shx");
		if(shx_fname == NULL){
			LOG_ERROR("SHP_make_sf_name failed for \"shx\" file");
			err = 1;
			goto CLEAN_UP;
		}

		shp_fhdr = SHP_open_file(shp_fname);
		if(shp_fhdr == NULL){
			LOG_ERROR("SHP_open_file failed for %s", shp_fname);
			err = 1;
			goto CLEAN_UP;
		}
		if(verbose)
			SHP_dump_fhdr(stderr, shp_fhdr);
		if(shp_fhdr->s_type == ST_NULL){
			LOG_WARN("shape file %s contains only NULL objects, nothing to do", shp_fname);
			err = 1;
			goto CLEAN_UP;
		}
	}else{		// data in a file map
	}

	if(pfname != NULL){
		// TODO: Fix so p_key can be int or str depending is the data is from -sf S or -fmap F
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
	}

	if(SHP_read_shx_data(shx_fname, verbose, &n_recs, &ridx)){
		LOG_ERROR("SHP_read_shx_data failed for %s", shx_fname);
		err = 1;
		goto CLEAN_UP;
	}

	for(a_prlg = first = 1, lcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_WARN("line %7d: empty line", lcnt);
				continue;
			}
		}
		rnum = atoi(line);
		if(rnum < 1 || rnum > n_recs){
			LOG_WARN("line %7d: rnum %d out of range 1:%d", lcnt, rnum, n_recs);
			err = 1;
			continue;
		}
		rip = &ridx[rnum - 1];
		fseek(shp_fhdr->s_fp, SF_WORD_SIZE * rip->s_offset, SEEK_SET);
		shp = SHP_read_shape(shp_fhdr->s_fp);
		if(shp == NULL){
			LOG_ERROR("line %7d: SHP_read_shape failed for record %d", lcnt, rnum);
			err = 1;
			goto CLEAN_UP;
		}
		if(verbose)
			SHP_dump_shape(stderr, shp, verbose);
		if(a_prlg){
			a_prlg = 0;
			SHP_write_geojson_prolog(stdout, fmt);
		}
		p_value = NULL;
		if(props != NULL){
			pp = PROPS_find_props_with_int_key(props, shp->s_rnum);
			if(pp == NULL){
				LOG_WARN("no properties for rnum = %d", shp->s_rnum);
				err = 1;
			}else{
				p_value = PROPS_to_json_object(props, pp);
				if(p_value == NULL){
					LOG_ERROR("PROPS_to_json_object failed");
					err = 1;
					goto CLEAN_UP;
				}
			}
		}
		SHP_write_geojson(stdout, shp, first, p_value);
		SHP_delete_shape(shp);
		shp = NULL;
		if(p_value != NULL){
			free(p_value);
			p_value = NULL;
		}
		first = 0;
	}
	if(!a_prlg)
		SHP_write_geojson_trailer(stdout, fmt);

CLEAN_UP : ;

	if(shp != NULL)
		SHP_delete_shape(shp);

	if(line != NULL)
		free(line);

	if(props != NULL)
		PROPS_delete_properties(props);

	if(ridx != NULL)
		free(ridx);

	if(shx_fname != NULL)
		free(shx_fname);

	if(shp_fhdr != NULL)
		SHP_close_file(shp_fhdr);
	if(shp_fname != NULL)
		free(shp_fname);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}
