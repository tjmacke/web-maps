#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <openssl/evp.h>

#include "log.h"
#include "args.h"
#include "props.h"
#include "shape.h"
#include "s2g_input.h"
#include "index.h"
#include "fmap.h"

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

static	KEY2RNUM_T	*
findkey(const char *, int , KEY2RNUM_T []);

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
	char	*p_value;

	S2G_INPUT_T	*s2g = NULL;
	char	*mvp;
	KEY2RNUM_T	*i2r;
	RIDX_T	ridx;
	long	offset;
	SF_SHAPE_T	*shp = NULL;

	FILE	*fp = NULL;
	char	*line = NULL;
	size_t	s_line = 0;
	ssize_t	l_line;
	int	lcnt;
	int	i, rnum;
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

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read rnum file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	s2g = S2G_new(verbose, sf, fm_name);
	if(s2g == NULL){
		LOG_ERROR("S2G_new failed");
		err = 1;
		goto CLEAN_UP;
	}

/*
	if(sf != NULL){	// data in a single shp/shx pair
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

		if(SHP_read_shx_data(shx_fname, verbose, &n_recs, &sf_ridx)){
			LOG_ERROR("SHP_read_shx_data failed for %s", shx_fname);
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
		// TODO: collect all of this into a func
		OpenSSL_add_all_digests();
		md = EVP_get_digestbyname(MD_NAME);
		if(md == NULL){
			LOG_ERROR("no such digest %s", MD_NAME);
			err = 1;
			goto CLEAN_UP;
		}
		EVP_MD_CTX_init(&mdctx);
		mdctx_init = 1;
	
		// TODO: collect all of this into a func
		if((fmap = FMread_fmap(fm_name)) == NULL){
			LOG_ERROR("FMread_fmap failed for fie map %s", fm_name);
			err = 1;
			goto CLEAN_UP;
		}
		if((i2rfp = FMfopen(fmap->f_root, "", fmap->f_index, "r")) == NULL){
			LOG_ERROR("FMfopen failed for rec index file %s", fmap->f_index);
			err = 1;
			goto CLEAN_UP;
		}
		i2rfd = fileno(i2rfp);
		if(fstat(i2rfd, &i2rsbuf)){
			LOG_ERROR("can't stat rec index file %s", fmap->f_index);
			err = 1;
			goto CLEAN_UP;
		}
		map = mmap(NULL, i2rsbuf.st_size, PROT_READ, MAP_SHARED, i2rfd, 0L);
		if(map == NULL){
			LOG_ERROR("can't mmap rec index file %s", fmap->f_index);
			err = 1;
			goto CLEAN_UP;
		}
		i2rhdr = (HDR_T *)map;
		n_atab = i2rhdr->h_count;
		atab = (KEY2RNUM_T *)&((char *)map)[sizeof(HDR_T)];
	}
*/

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
	}else if(fm_name != NULL){
		// need some sort of props file as the default title shape_%07d needs to be unique
		props = PROPS_make_default_ptab("title");
		if(props == NULL){
			LOG_ERROR("PROPS_make_default_ptab failed");
			err = 1;
			goto CLEAN_UP;
		}
	}

	for(s2g->sl_fme = NULL, a_prlg = first = 1, lcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
		lcnt++;
		if(line[l_line - 1] == '\n'){
			line[l_line - 1] = '\0';
			l_line--;
			if(l_line == 0){
				LOG_WARN("line %7d: empty line", lcnt);
				continue;
			}
		}
		if(sf != NULL){
			rnum = atoi(line);
			if(rnum < 1 || rnum > s2g->sn_recs){
				LOG_WARN("line %7d: rnum %d out of range 1:%d", lcnt, rnum, s2g->sn_recs);
				err = 1;
				continue;
			}
			s2g->s_sf_rip = &s2g->s_sf_ridx[rnum - 1];
			fseek(s2g->s_shp_fhdr->s_fp, SF_WORD_SIZE * s2g->s_sf_rip->s_offset, SEEK_SET);
		}else{	// using fmap
			EVP_DigestInit_ex(&s2g->s_mdctx, s2g->s_md, NULL);
			EVP_DigestUpdate(&s2g->s_mdctx, line, l_line);
			EVP_DigestFinal(&s2g->s_mdctx, s2g->s_md_value, &s2g->s_md_len);
			for(mvp = s2g->s_md_value_str, i = 0; i < s2g->s_md_len; i++, mvp += 2)
				sprintf(mvp, "%02x", s2g->s_md_value[i] & 0xff);
			*mvp = '\0';

			i2r = findkey(s2g->s_md_value_str, s2g->sn_atab, s2g->s_atab);
			if(i2r == NULL){
				LOG_ERROR("%s (%s) not in index %s", line, s2g->s_md_value_str, s2g->s_fmap->f_index);
				err = 1;
				continue;
			}

			if((s2g->s_fme = FMrnum2fmentry(s2g->s_fmap, i2r->k_rnum)) == NULL){
				LOG_ERROR("NO fme for rnum %010u", i2r->k_rnum);
				err = 1;
				goto CLEAN_UP;
			}
			if(s2g->s_fme != s2g->sl_fme){
				if(s2g->s_rixfp != NULL){
					fclose(s2g->s_rixfp);
					s2g->s_rixfp = NULL;
				}
				if((s2g->s_rixfp = FMfopen(s2g->s_fmap->f_root, s2g->s_fme->f_fname, s2g->s_fmap->f_ridx, "r")) == NULL){
					LOG_ERROR("FMfopen failed for %s", s2g->s_fme->f_fname);
					err = 1;
					goto CLEAN_UP;
				}
				fread(&s2g->s_rixhdr, sizeof(HDR_T), (size_t)1, s2g->s_rixfp);
				if(s2g->s_shp_fhdr != NULL){
					SHP_close_file(s2g->s_shp_fhdr);
					s2g->s_shp_fhdr = NULL;
				}
				// TODO: open new one
				s2g->sl_fm_dfname = strlen(s2g->s_fmap->f_root) + 1 + strlen(s2g->s_fme->f_fname); 
				if(s2g->sl_fm_dfname + 1 > s2g->ss_fm_dfname){
					if(s2g->s_fm_dfname != NULL)
						free(s2g->s_fm_dfname);
					s2g->ss_fm_dfname = s2g->sl_fm_dfname + 1;
					s2g->s_fm_dfname = (char *)malloc(s2g->ss_fm_dfname);
					if(s2g->s_fm_dfname == NULL){
						LOG_ERROR("can't allocate fm_dfname for %s/%s", s2g->s_fmap->f_root, s2g->s_fme->f_fname);
						err = 1;
						goto CLEAN_UP;
					}
				}
				sprintf(s2g->s_fm_dfname, "%s/%s", s2g->s_fmap->f_root, s2g->s_fme->f_fname);
				s2g->s_shp_fhdr = SHP_open_file(s2g->s_fm_dfname);
				if(s2g->s_shp_fhdr == NULL){
					LOG_ERROR("SHP_open_file failed for %s", s2g->s_fme->f_fname);
					err = 1;
					goto CLEAN_UP;
				}
			}

			// get the offset (as bytes this time for the shp to read)
			offset = (i2r->k_rnum - s2g->s_fme->f_first) * s2g->s_rixhdr.h_size + sizeof(HDR_T);
			fseek(s2g->s_rixfp, (long)offset, SEEK_SET);
			fread(&ridx, sizeof(ridx), 1L, s2g->s_rixfp);

			fseek(s2g->s_shp_fhdr->s_fp, ridx.r_offset, SEEK_SET);
		}
		shp = SHP_read_shape(s2g->s_shp_fhdr->s_fp);
		if(shp == NULL){
			LOG_ERROR("line %7d: SHP_read_shape failed for record %d", lcnt, rnum);
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
		p_value = NULL;
		if(props != NULL){
			if(sf != NULL){
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
			}else{
				pp = PROPS_find_props_with_str_key(props, line);
				if(pp == NULL){
					def_pp = PROPS_new_prop("title", 0, line);
					if(def_pp == NULL){
						LOG_ERROR("PROPS_new_prop failed for %s", line);
						err = 1;
						goto CLEAN_UP;
					}
					p_value = PROPS_to_json_object(props, def_pp);
					PROPS_delete_prop(def_pp);
					def_pp = NULL;
				}else
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

/*
	// file map stuff
	if(fm_dfname != NULL)
		free(fm_dfname);
	if(mdctx_init)
		EVP_MD_CTX_cleanup(&mdctx);
	if(map != NULL){
		munmap(map, i2rsbuf.st_size);
		map = NULL;
	}
	if(fmap != NULL)
		FMfree_fmap(fmap);

	// shape file stuff
	if(sf_ridx != NULL)
		free(sf_ridx);
	if(shx_fname != NULL)
		free(shx_fname);
	if(shp_fhdr != NULL)
		SHP_close_file(shp_fhdr);
	if(shp_fname != NULL)
		free(shp_fname);
*/
	if(s2g != NULL)
		S2G_delete(s2g);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}

static	KEY2RNUM_T	*
findkey(const char *key, int n_idx, KEY2RNUM_T idx[])
{
	int	i, j, k, cv;
	KEY2RNUM_T	*k2r;

	for(i = 0, j = n_idx - 1; i <= j; ){
		k = (i + j) / 2;
		k2r = &idx[k];
		if((cv = strcmp(k2r->k_key, key)) == 0)
			return k2r;
		else if(cv < 0)
			i = k + 1;
		else
			j = k - 1;
	}
	return NULL;
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
