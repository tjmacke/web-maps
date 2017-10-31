#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "dbase.h"

DBF_FHDR_T	*
DBF_new_fhdr(void)
{
	DBF_FHDR_T	*fhdr = NULL;
	int	err = 0;

	fhdr = (DBF_FHDR_T *)calloc((size_t)1, sizeof(DBF_FHDR_T));
	if(fhdr == NULL){
		LOG_ERROR("can't allocate fhdr");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(err){
		DBF_delete_fhdr(fhdr);
		fhdr = NULL;
	}

	return fhdr;
}

int
DBF_read_fhdr(FILE *fp, DBF_FHDR_T *fhdr)
{
	char	buf[DBF_FHDR_SIZE], *bp;
	int	n_read;
	int	err = 0;

	n_read = fread(buf, sizeof(char), DBF_FHDR_SIZE, fp);
	if(n_read != DBF_FHDR_SIZE){
		LOG_ERROR("short hdr: %d bytes, need %d", n_read, DBF_FHDR_SIZE);
		err = 1;
		goto CLEAN_UP;
	}
	fhdr->d_version = buf[DBF_FHDR_VERSION];
	bp = &buf[DBF_FHDR_DATE];
	sprintf(fhdr->d_date, "%04d-%02d-%02d", (*bp & 0xff) + 1900, (bp[1] & 0xff), (bp[2] & 0xff));
	bp = &buf[DBF_FHDR_NRECS];
	fhdr->dn_recs = (*bp & 0xff) | ((bp[1] & 0xff) << 8) | ((bp[2] & 0xff) << 16) | ((bp[3] & 0xff) << 24);
	bp = &buf[DBF_FHDR_LHDR];
	fhdr->dl_hdr = (*bp & 0xff) | ((bp[1] & 0xff) << 8);
	bp = &buf[DBF_FHDR_LREC];
	fhdr->dl_rec = (*bp & 0xff) | ((bp[1] & 0xff) << 8);
	fhdr->d_encrypted = buf[DBF_FHDR_ENCRYPTED];
	fhdr->d_lang_info = buf[DBF_FHDR_LANG_INFO];

CLEAN_UP : ;

	return err;
}

void
DBF_dump_fhdr(FILE *fp, DBF_FHDR_T *fhdr)
{

	if(fhdr == NULL){
		fprintf(fp, "fdhr = NULL\n");
		return;
	}

	fprintf(fp, "fhdr = {\n");
	fprintf(fp, "\tversion     = %d\n", fhdr->d_version);
	fprintf(fp, "\tdate        = %s\n", fhdr->d_date);
	fprintf(fp, "\tn recs      = %d\n", fhdr->dn_recs);
	fprintf(fp, "\tl_hdr       = %d\n", fhdr->dl_hdr);
	fprintf(fp, "\tl_rec       = %d\n", fhdr->dl_rec);
	fprintf(fp, "\tencrypted   = %d\n", fhdr->d_encrypted);
	fprintf(fp, "\tlang_info   = 0x%02x\n", fhdr->d_lang_info);
	fprintf(fp, "}\n");
}

void
DBF_delete_fhdr(DBF_FHDR_T *fhdr)
{

	if(fhdr == NULL)
		return;

	free(fhdr);
}
