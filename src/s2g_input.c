#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "shape.h"
#include "s2g_input.h"

S2G_INPUT_T	*
S2G_new(int verbose, const char *sf, const char *fm_name)
{
	int	u_sf = 0;
	int	u_fmap = 0;
	S2G_INPUT_T	*s2g = NULL;
	int	err = 0;

	if(sf != NULL && *sf != '\0')
		u_sf = 1;
	if(fm_name != NULL && *fm_name != '\0')
		u_fmap = 1;
	if(!u_sf && !u_fmap){
		LOG_ERROR("one of sf, fmap must be set.");
		err = 1;
		goto CLEAN_UP;
	}
	if(u_sf && u_fmap){
		LOG_ERROR("only one of sf, famp can be set.");
		err = 1;
		goto CLEAN_UP;
	}

	s2g = (S2G_INPUT_T *)calloc((size_t)1, sizeof(S2G_INPUT_T));
	if(s2g == NULL){
		LOG_ERROR("can't allocate s2g");
		err = 1;
		goto CLEAN_UP;
	}
	s2g->s_verbose = verbose;
	s2g->s_sf = sf;
	s2g->s_fm_name = fm_name;
	if(u_sf){	// data in a single shp/shx pair
		s2g->s_shp_fname = SHP_make_sf_name(s2g->s_sf, "shp");
		if(s2g->s_shp_fname == NULL){
			LOG_ERROR("SHP_make_sf_name failed for \"shp\" file.");
			err = 1;
			goto CLEAN_UP;
		}
		s2g->s_shx_fname = SHP_make_sf_name(s2g->s_sf, "shx");
		if(s2g->s_shx_fname == NULL){
			LOG_ERROR("SHP_make_sf_name failed for \"shx\" file.");
			err = 1;
			goto CLEAN_UP;
		}

		if(SHP_read_shx_data(s2g->s_shx_fname, s2g->s_verbose, &s2g->sn_recs, &s2g->s_sf_ridx)){
			LOG_ERROR("SHP_read_shx_data failed for %s.", s2g->s_shx_fname);
			err = 1;
			goto CLEAN_UP;
		}

		s2g->s_shp_fhdr = SHP_open_file(s2g->s_shp_fname);
		if(s2g->s_shp_fhdr == NULL){
			LOG_ERROR("SHP_open_file failed for %s.", s2g->s_shp_fname);
			err = 1;
			goto CLEAN_UP;
		}
		if(s2g->s_shp_fhdr->s_type == ST_NULL){
			LOG_WARN("shape file %s contains only NULL objects, nothing to do.", s2g->s_shp_fname);
			err = 1;
			goto CLEAN_UP;
		}
	}else{	// data in a file map
		OpenSSL_add_all_digests();
		s2g->s_md = EVP_get_digestbyname(MD_NAME);
		if(s2g->s_md == NULL){
			LOG_ERROR("no such digests %s.", MD_NAME);
			err = 1;
			goto CLEAN_UP;
		}
		EVP_MD_CTX_init(&s2g->s_mdctx);
		s2g->s_mdctx_init = 1;

		if((s2g->s_fmap = FMread_fmap(s2g->s_fm_name)) == NULL){
			LOG_ERROR("FMread_fmap failed for file map %s.", s2g->s_fm_name);
			err = 1;
			goto CLEAN_UP;
		}
		if((s2g->s_i2rfp = FMfopen(s2g->s_fmap->f_root, "", s2g->s_fmap->f_index, "r")) == NULL){
			LOG_ERROR("FMfopen failed for rec index file %s.", s2g->s_fmap->f_index);
			err = 1;
			goto CLEAN_UP;
		}
		s2g->s_i2rfd = fileno(s2g->s_i2rfp);
		if(fstat(s2g->s_i2rfd, &s2g->s_i2rsbuf)){
			LOG_ERROR("can't stat rec index file %s.", s2g->s_fmap->f_index);
			err = 1;
			goto CLEAN_UP;
		}
		s2g->s_map = mmap(NULL, s2g->s_i2rsbuf.st_size, PROT_READ, MAP_SHARED, s2g->s_i2rfd, 0L);
		if(s2g->s_map == NULL){
			LOG_ERROR("can't mmap rec index file %s.", s2g->s_fmap->f_index);
			err = 1;
			goto CLEAN_UP;
		}
		s2g->s_i2rhdr = (HDR_T *)s2g->s_map;
		s2g->sn_atab = s2g->s_i2rhdr->h_count;
		s2g->s_atab = (KEY2RNUM_T *)&((char *)s2g->s_map)[sizeof(HDR_T)];
	}

CLEAN_UP : ;

	if(err){
		if(s2g != NULL){
			S2G_delete(s2g);
			s2g = NULL;
		}
	}

	return s2g;
}

void
S2G_delete(S2G_INPUT_T *s2g)
{

	if(s2g != NULL){
		if(s2g->s_sf != NULL){
			if(s2g->s_shp_fhdr != NULL)
				SHP_close_file(s2g->s_shp_fhdr);
			if(s2g->s_sf_ridx != NULL)
				free(s2g->s_sf_ridx);
			if(s2g->s_shx_fname != NULL)
				free(s2g->s_shx_fname);
			if(s2g->s_shp_fname != NULL)
				free(s2g->s_shp_fname);
		}else{
			if(s2g->s_map != NULL)
				munmap(s2g->s_map, s2g->s_i2rsbuf.st_size);
			if(s2g->s_fmap != NULL)
				FMfree_fmap(s2g->s_fmap);
			if(s2g->s_mdctx_init)
				EVP_MD_CTX_cleanup(&s2g->s_mdctx);
		}
		free(s2g);
	}
}
