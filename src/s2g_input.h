#ifndef	_S2G_INPUT_H_
#define	_S2G_INPUT_H_

#include <sys/stat.h>
#include <sys/mman.h>
#include <openssl/evp.h>
#include "shape.h"
#include "fmap.h"
#include "index.h"

#define	MD_NAME	"md5"

typedef	struct	s2g_input_t	{
	int	s_verbose;

	// record selection: only will be active
	int	s_all;	// all records in the collection 
	FILE	*s_fp;	// records selection in this file

	// geom input is either
	// 1. a single shp/shx pair
	const char	*s_sf;
	char	*s_shp_fname;
	SF_FHDR_T	*s_shp_fhdr;
	char	*s_shx_fname;
	int	sn_recs;
	SF_RIDX_T	*s_sf_ridx;
	SF_RIDX_T	*s_sf_rip;
	// the all "cursor"
	int	sc_rnum;

	// or
	// 2. a shp/shx collection described by fmap
	const char	*s_fm_name;
	FMAP_T	*s_fmap;
	FILE	*s_i2rfp;
	int	s_i2rfd;
	struct stat	s_i2rsbuf;
	void	*s_map;
	HDR_T	*s_i2rhdr;
	KEY2RNUM_T	*s_atab;
	int	sn_atab;
	FM_ENTRY_T	*sl_fme, *s_fme;
	FILE	*s_rixfp;
	HDR_T	s_rixhdr;
	char	*s_fm_dfname;
	size_t	ss_fm_dfname;
	size_t	sl_fm_dfname;

	// strings are used to select from fmap; use md5 digest to fit them into 32 chars
	const EVP_MD	*s_md;
	EVP_MD_CTX	s_mdctx;
	int	s_mdctx_init;
	unsigned char	s_md_value[EVP_MAX_MD_SIZE];
	unsigned int	s_md_len;
	char	s_md_value_str[(2*EVP_MAX_MD_SIZE + 1)];
} S2G_INPUT_T;

S2G_INPUT_T	*
S2G_new(int, const char *, const char *, int, FILE *);

void
S2G_delete(S2G_INPUT_T *);

ssize_t
S2G_getline(S2G_INPUT_T *, char **, size_t *);

SF_SHAPE_T	*
S2G_get_shape(S2G_INPUT_T *, const char *);

#endif
