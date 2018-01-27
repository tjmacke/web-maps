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
#include "index.h"
#include "fmap.h"

#define	MD_NAME	"md5"

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

static	KEY2RNUM_T	*
findkey(const char *, int , KEY2RNUM_T []);

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

	char	*shp_fname = NULL;
	SF_FHDR_T	*shp_fhdr = NULL;
	char	*shx_fname = NULL;
	int	n_recs = 0;
	SF_RIDX_T	*sf_ridx = NULL;
	SF_RIDX_T	*sf_rip;
	SF_SHAPE_T	*shp = NULL;

	const EVP_MD	*md = NULL;
	EVP_MD_CTX	mdctx;
	int	mdctx_init = 0;
	unsigned char	md_value[EVP_MAX_MD_SIZE];
	unsigned int	md_len;
	char	md_value_str[(2*EVP_MAX_MD_SIZE + 1)];
	char	*mvp;

	FMAP_T	*fmap = NULL;
	FILE	*i2rfp = NULL;
	int	i2rfd;
	struct stat	i2rsbuf;
	void	*map = NULL;
	HDR_T	*i2rhdr;
	KEY2RNUM_T	*atab, *i2r;
	int	n_atab;
	FM_ENTRY_T	*l_fme, *fme;
	FILE	*rixfp = NULL;
	HDR_T	rixhdr;
	RIDX_T	ridx;
	char	*fm_dfname = NULL;
	size_t	s_fm_dfname = 0;
	size_t	l_fm_dfname = 0;
	long	offset;

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
	}

	for(l_fme = NULL, a_prlg = first = 1, lcnt = 0; (l_line = getline(&line, &s_line, fp)) > 0; ){
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
			if(rnum < 1 || rnum > n_recs){
				LOG_WARN("line %7d: rnum %d out of range 1:%d", lcnt, rnum, n_recs);
				err = 1;
				continue;
			}
			sf_rip = &sf_ridx[rnum - 1];
			fseek(shp_fhdr->s_fp, SF_WORD_SIZE * sf_rip->s_offset, SEEK_SET);
		}else{	// using fmap
			EVP_DigestInit_ex(&mdctx, md, NULL);
			EVP_DigestUpdate(&mdctx, line, l_line);
			EVP_DigestFinal(&mdctx, md_value, &md_len);
			for(mvp = md_value_str, i = 0; i < md_len; i++, mvp += 2)
				sprintf(mvp, "%02x", md_value[i] & 0xff);
			*mvp = '\0';

			i2r = findkey(md_value_str, n_atab, atab);
			if(i2r == NULL){
				LOG_ERROR("%s (%s) not in index %s", line, md_value_str, fmap->f_index);
				err = 1;
				continue;
			}

			if((fme = FMrnum2fmentry(fmap, i2r->k_rnum)) == NULL){
				LOG_ERROR("NO fme for rnum %010u", i2r->k_rnum);
				err = 1;
				goto CLEAN_UP;
			}
			if(fme != l_fme){
				if(rixfp != NULL){
					fclose(rixfp);
					rixfp = NULL;
				}
				if((rixfp = FMfopen(fmap->f_root, fme->f_fname, fmap->f_ridx, "r")) == NULL){
					LOG_ERROR("FMfopen failed for %s", fme->f_fname);
					err = 1;
					goto CLEAN_UP;
				}
				fread(&rixhdr, sizeof(HDR_T), (size_t)1, rixfp);
				if(shp_fhdr != NULL){
					SHP_close_file(shp_fhdr);
					shp_fhdr = NULL;
				}
				// TODO: open new one
				l_fm_dfname = strlen(fmap->f_root) + 1 + strlen(fme->f_fname); 
				if(l_fm_dfname + 1 > s_fm_dfname){
					if(fm_dfname != NULL)
						free(fm_dfname);
					s_fm_dfname = l_fm_dfname + 1;
					fm_dfname = (char *)malloc(s_fm_dfname);
					if(fm_dfname == NULL){
						LOG_ERROR("can't allocate fm_dfname for %s/%s", fmap->f_root, fme->f_fname);
						err = 1;
						goto CLEAN_UP;
					}
				}
				sprintf(fm_dfname, "%s/%s", fmap->f_root, fme->f_fname);
				shp_fhdr = SHP_open_file(fm_dfname);
				if(shp_fhdr == NULL){
					LOG_ERROR("SHP_open_file failed for %s", fme->f_fname);
					err = 1;
					goto CLEAN_UP;
				}
			}

			// get the offset (as bytes this time for the shp to read)
			offset = (i2r->k_rnum - fme->f_first) * rixhdr.h_size + sizeof(HDR_T);
			fseek(rixfp, (long)offset, SEEK_SET);
			fread(&ridx, sizeof(ridx), 1L, rixfp);

			fseek(shp_fhdr->s_fp, ridx.r_offset, SEEK_SET);
		}
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
			pp = (sf != NULL) ? PROPS_find_props_with_int_key(props, shp->s_rnum) : PROPS_find_props_with_str_key(props, line);
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
