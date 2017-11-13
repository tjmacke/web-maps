#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message.h"},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-sf",   0, AVK_REQ,  AVT_STR,  NULL, "Use -sf S to use convert that shapes in S.shp, S.shx to geojson."},
	{"-pf",   1, AVK_REQ,  AVT_STR,  NULL, "Use -pf P to to add properties to the geojson."},
	{"-all",  1, AVK_NONE, AVT_BOOL, "0",  "Use -all to convert all records, else convert only records whose rnums are in file."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

static	char	*
mk_sf_name(const char *, const char *);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	const char	*sf = NULL;
	const char	*pfname = NULL;
	FILE	*pfp = NULL;
	char	*shp_fname = NULL;
	FILE	*shp_fp = NULL;
	SF_FHDR_T	*shp_fhdr = NULL;
	char	*shx_fname = NULL;
	FILE	*shx_fp = NULL;
	SF_FHDR_T	*shx_fhdr = NULL;
	int	all = 0;
	int	n_recs;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-sf", AVT_STR);
	sf = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-pf", AVT_STR);
	pfname = a_val->av_value.v_str;

	a_val = TJM_get_flag_value(args, "-all", AVT_BOOL);
	all = a_val->av_value.v_int;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	shp_fname = mk_sf_name(sf, "shp");
	if(shp_fname == NULL){
		LOG_ERROR("mk_sfname failed for shp file");
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

	shx_fname = mk_sf_name(sf, "shx");
	if(shx_fname == NULL){
		LOG_ERROR("mk_sf_name failed for shx file");
		err = 1;
		goto CLEAN_UP;
	}
	shx_fhdr = SHP_open_file(shx_fname);
	if(shx_fhdr == NULL){
		LOG_ERROR("SHP_open_file failed for %s", shx_fname);
		err = 1;
		goto CLEAN_UP;
	}
	if(verbose)
		SHP_dump_fhdr(stderr, shx_fhdr);

	n_recs = (shx_fhdr->sl_file - SF_FHDR_SIZE) / SF_RIDX_SIZE;

LOG_DEBUG("%s: %d recs", shp_fname, n_recs);

	if(pfname != NULL){
		if((pfp = fopen(pfname, "r")) == NULL){
			LOG_ERROR("can't read property file %s", pfname);
			err = 1;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;

	if(pfp != NULL)
		fclose(pfp);

	if(shx_fhdr != NULL)
		SHP_close_file(shx_fhdr);
	if(shx_fname != NULL)
		free(shx_fname);

	if(shp_fhdr != NULL)
		SHP_close_file(shp_fhdr);
	if(shp_fname != NULL)
		free(shp_fname);

	TJM_free_args(args);

	exit(err);
}

static	char	*
mk_sf_name(const char *pfx, const char *ext)
{
	char	*sfname = NULL;
	size_t	s_sfname = 0;
	char	*sfp;
	const char	*pp, *slash, *dot;
	int	add_dot = 0;
	int	add_ext = 0;
	int	err = 0;

	if(pfx == NULL || *pfx == '\0'){
		LOG_ERROR("pfx is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	if(ext == NULL || *ext == '\0'){
		LOG_ERROR("ext is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	for(dot = slash = NULL, pp = pfx; *pp; pp++){
		if(*pp == '/')
			slash = pp;
		else if(*pp == '.')
			dot = pp;
	}

	if(slash != NULL){
		if(slash[1] == '\0'){
			LOG_ERROR("pfx %s is a directory", pfx);
			err = 1;
			goto CLEAN_UP;
		}
		
		if(dot < slash)
			add_dot = add_ext = 1;
		else if(dot[1] == '\0')
			add_ext = 1;
		else if(strcmp(&dot[1], ext))
			add_dot = add_ext = 1;
		// pfx is alread correct, just copy it
	}else if(dot != NULL){
		if(dot[1] == '\0')
			add_ext = 1;
		else if(strcmp(&dot[1], ext))
			add_dot = add_ext = 1;
		// pfx is alread correct, just copy it
	}else
		add_dot = add_ext = 1;

	s_sfname = strlen(pfx);
	if(add_dot)
		s_sfname++;
	if(add_ext)
		s_sfname += strlen(ext);
	s_sfname++;	// don't forget the final \0

	sfname = (char *)malloc(s_sfname * sizeof(char));
	if(sfname == NULL){
		LOG_ERROR("can't allocate sfname");
		err = 1;
		goto CLEAN_UP;
	}
	strcpy(sfname, pfx);
	sfp = &sfname[pp - pfx];
	if(add_dot)	// add_dot implies add_ext
		*sfp++ = '.';
	if(add_ext)
		strcpy(sfp, ext);

CLEAN_UP : ;

	if(err){
		if(sfname != NULL){
			free(sfname);
			sfname = NULL;
		}
	}

	return sfname;
}
