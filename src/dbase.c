#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "dbase.h"

DBF_META_T	*
DBF_new_dbf_meta(const char *fname)
{
	DBF_META_T	*dbm = NULL;
	int	err = 0;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	dbm = (DBF_META_T *)calloc((size_t)1, sizeof(DBF_META_T));
	if(dbm == NULL){
		LOG_ERROR("can't allocate dbm");
		err = 1;
		goto CLEAN_UP;
	}

	dbm->d_fname = strdup(fname);
	if(dbm->d_fname == NULL){
		LOG_ERROR("can't strdup fname");
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(err){
		DBF_delete_dbf_meta(dbm);
		dbm = NULL;
	}

	return dbm;
}

void
DBF_delete_dbf_meta(DBF_META_T *dbm)
{

	if(dbm == NULL)
		return;

	if(dbm->d_fname != NULL)
		free(dbm->d_fname);

	if(dbm->d_fhdr != NULL)
		DBF_delete_fhdr(dbm->d_fhdr);

	if(dbm->d_fields != NULL){
		int	i;
		for(i = 0; i < dbm->dn_fields; i++){
			if(dbm->d_fields[i] != NULL){
				free(dbm->d_fields[i]);
			}
		}
		free(dbm->d_fields);
	}

	free(dbm);
}

int
DBF_read_dbf_meta(FILE *fp, DBF_META_T *dbm)
{
	int	i, c;
	int	foff;
	DBF_FIELD_T	*fldp = NULL;
	int	err = 0;

	dbm->d_fhdr = DBF_read_fhdr(fp);
	if(dbm->d_fhdr == NULL){
		LOG_ERROR("DBF_read_fhdr failed");
		err = 1;
		goto CLEAN_UP;
	}
	dbm->dn_fields = (dbm->d_fhdr->dl_hdr - DBF_FHDR_SIZE - 1) / DBF_FIELD_SIZE;
	dbm->d_fields = (DBF_FIELD_T **)calloc((size_t)dbm->dn_fields, sizeof(DBF_FIELD_T *));
	if(dbm->d_fields == NULL){
		LOG_ERROR("can't allocate d_fields");
		err = 1;
		goto CLEAN_UP;
	}
	for(foff= i = 0; i < dbm->dn_fields; i++){
		fldp = DBF_read_field(fp);
		if(fldp == NULL){
			LOG_ERROR("DBF_read_field failed for field %d", i+1);
			err = 1;
			goto CLEAN_UP;
		}
		fldp->d_addr = foff;
		foff += fldp->d_length;
		dbm->d_fields[i] = fldp;
	}
	if((c = getc(fp)) == EOF){
		LOG_ERROR("missing header terminator");
		err = 1;
		goto CLEAN_UP;
	}else if(c != '\x0d'){
		LOG_ERROR("bad header terminator %c, expect 0x%02x", c, '\x0d');
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	return err;
}

void
DBF_dump_dbf_meta(FILE *fp, DBF_META_T *dbm, int verbose)
{
	int	i;

	if(dbm == NULL){
		fprintf(fp, "dbm is NULL\n");
		return;
	}

	fprintf(fp, "dbf_meta = {\n");
	fprintf(fp, "\tfname   = %s\n", dbm->d_fname ? dbm->d_fname : "NULL");
	DBF_dump_fhdr(fp, dbm->d_fhdr, "\t");
	fprintf(fp, "\tn_fields = %d\n", dbm->dn_fields);
	fprintf(fp, "\tfields = {\n");
	for(i = 0; i < dbm->dn_fields; i++)
		DBF_dump_field(fp, dbm->d_fields[i], verbose, i+1, "\t\t");
	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");
}

DBF_FHDR_T	*
DBF_read_fhdr(FILE *fp)
{
	DBF_FHDR_T	*fhdr = NULL;
	char	buf[DBF_FHDR_SIZE], *bp;
	int	n_read;
	int	err = 0;

	fhdr = (DBF_FHDR_T *)calloc((size_t)1, sizeof(DBF_FHDR_T));
	if(fhdr == NULL){
		LOG_ERROR("can't allocate fhdr");
		err = 1;
		goto CLEAN_UP;
	}

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

	if(err){
		DBF_delete_fhdr(fhdr);
		fhdr = NULL;
	}

	return fhdr;
}

void
DBF_delete_fhdr(DBF_FHDR_T *fhdr)
{

	if(fhdr == NULL)
		return;

	free(fhdr);
}
void
DBF_dump_fhdr(FILE *fp, DBF_FHDR_T *fhdr, const char *indent)
{

	if(fhdr == NULL){
		fprintf(fp, "%sfdhr = NULL\n", indent ? indent : "");
		return;
	}

	fprintf(fp, "%sfhdr = {\n", indent ? indent : "");
	fprintf(fp, "%s\tversion     = %d\n", indent ? indent : "", fhdr->d_version);
	fprintf(fp, "%s\tdate        = %s\n", indent ? indent : "", fhdr->d_date);
	fprintf(fp, "%s\tn recs      = %d\n", indent ? indent : "", fhdr->dn_recs);
	fprintf(fp, "%s\tl_hdr       = %d\n", indent ? indent : "", fhdr->dl_hdr);
	fprintf(fp, "%s\tl_rec       = %d\n", indent ? indent : "", fhdr->dl_rec);
	fprintf(fp, "%s\tencrypted   = %d\n", indent ? indent : "", fhdr->d_encrypted);
	fprintf(fp, "%s\tlang_info   = 0x%02x\n", indent ? indent : "", fhdr->d_lang_info);
	fprintf(fp, "%s}\n", indent ? indent : "");
}

DBF_FIELD_T	*
DBF_read_field(FILE *fp)
{
	DBF_FIELD_T	*fldp = NULL;
	char	buf[DBF_FIELD_SIZE], *bp;
	int	n_read;
	int	err = 0;

	fldp = (DBF_FIELD_T *)malloc(1 * sizeof(DBF_FIELD_T));
	if(fldp == NULL){
		LOG_ERROR("can't allocate fldp");
		err = 1;
		goto CLEAN_UP;
	}

	n_read = fread(buf, sizeof(char), DBF_FIELD_SIZE, fp);
	if(n_read != DBF_FIELD_SIZE){
		LOG_ERROR("bad field: expect %d bytes, got %d", DBF_FIELD_SIZE, n_read);
		err = 1;
		goto CLEAN_UP;
	}
	strcpy(fldp->d_name, buf);
	bp = &buf[DBF_FIELD_TYPE];
	fldp->d_type = *bp;
	bp = &buf[DBF_FIELD_ADDR];
	fldp->d_addr = ((*bp & 0xff) << 24) | ((bp[1] & 0xff) << 16) | ((bp[2] & 0xff) << 8) | (bp[3] & 0xff);
	fldp->d_length = buf[DBF_FIELD_LENGTH] & 0xff; 
	fldp->d_dec_count = buf[DBF_FIELD_DEC_COUNT] & 0xff;
	

CLEAN_UP : ;

	if(err){
		if(fldp != NULL)
			free(fldp);
		fldp = NULL;
	}

	return fldp;
}

void
DBF_dump_field(FILE *fp, DBF_FIELD_T *fldp, int verbose, int fnum, const char *indent)
{

	if(fldp == NULL){
		fprintf(fp, "%sfield %d is NULL\n", indent ? indent : "", fnum);
		return;
	}

	if(verbose){
		fprintf(fp, "%sfield = %d {\n", indent ? indent : "", fnum);
		fprintf(fp, "%s\tname      = %s\n", indent ? indent : "", fldp->d_name);
		fprintf(fp, "%s\ttype      = %c\n", indent ? indent : "", fldp->d_type);
		fprintf(fp, "%s\taddr      = %d\n", indent ? indent : "", fldp->d_addr);
		fprintf(fp, "%s\tlength    = %d\n", indent ? indent : "", fldp->d_length);
		fprintf(fp, "%s\tdec_count = %d\n", indent ? indent : "", fldp->d_dec_count);
		fprintf(fp, "%s}\n", indent ? indent : "");
	}else
		fprintf(fp, "%s%d: %s, %c, %d, %d, %d\n", indent ? indent : "", fnum,
			fldp->d_name, fldp->d_type, fldp->d_addr, fldp->d_length, fldp->d_dec_count);
}

void
DBF_dump_rec(FILE *fp, DBF_META_T *dbm, int verbose, int trim, int rnum, const char *rbuf)
{

	if(dbm == NULL){
		fprintf(fp, "dbm is NULL\n");
		return;
	}

	if(rbuf == NULL){
		fprintf(fp, "rbuf is NULL\n");
		return;
	}

	fprintf(fp, "rec = %d {\n", rnum);
	fprintf(fp, "}\n");
}
