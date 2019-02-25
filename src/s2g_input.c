#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "shape.h"
#include "s2g_input.h"
#include "index.h"
#include "fmap.h"

static	KEY2RNUM_T	*
findkey(const char *, int, KEY2RNUM_T []);

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

SF_SHAPE_T	*
S2G_get_shape(S2G_INPUT_T *s2g, const char *key)
{
	int	i, rnum;
	size_t	l_key;
	char	*mvp;
	KEY2RNUM_T	*i2r;
	RIDX_T	ridx;
	long	offset;
	SF_SHAPE_T	*shp = NULL;
	int	err = 0;

	if(s2g->s_sf != NULL){
		rnum = atoi(key);
		if(rnum < 1 || rnum > s2g->sn_recs){
			LOG_WARN("key: rnum %d out of range 1:%d", rnum, s2g->sn_recs);
			err = 1;
			goto CLEAN_UP;
		}
		s2g->s_sf_rip = &s2g->s_sf_ridx[rnum - 1];
		fseek(s2g->s_shp_fhdr->s_fp, SF_WORD_SIZE * s2g->s_sf_rip->s_offset, SEEK_SET);
	}else{	// using fmap
		l_key = strlen(key);
		EVP_DigestInit_ex(&s2g->s_mdctx, s2g->s_md, NULL);
		EVP_DigestUpdate(&s2g->s_mdctx, key, l_key);
		EVP_DigestFinal(&s2g->s_mdctx, s2g->s_md_value, &s2g->s_md_len);
		for(mvp = s2g->s_md_value_str, i = 0; i < s2g->s_md_len; i++, mvp += 2)
			sprintf(mvp, "%02x", s2g->s_md_value[i] & 0xff);
		*mvp = '\0';

		i2r = findkey(s2g->s_md_value_str, s2g->sn_atab, s2g->s_atab);
		if(i2r == NULL){
			LOG_ERROR("%s (%s) not in index %s", key, s2g->s_md_value_str, s2g->s_fmap->f_index);
			err = 1;
			goto CLEAN_UP;
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
			// open new one
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

CLEAN_UP : ;

	return shp;
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
