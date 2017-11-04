#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "args.h"
#include "shape.h"
#include "dbase.h"

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0", "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0", "Use -v to set the verbosity to 1; use -v=N to set it to N."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	int	ftype = SFT_UNKNOWN;
	FILE	*fp = NULL;
	SF_FHDR_T	*fhdr = NULL;
	DBF_META_T	*dbm = NULL;
	int	n_fields;
	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 1, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	ftype = SHP_get_file_type(args->a_files[0]);
	if(ftype == SFT_UNKNOWN){
		LOG_ERROR("SHP_get_file_type failed for %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read input file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	switch(ftype){
	case SFT_SHP :
	case SFT_SHX :
		fhdr = SHP_new_fhdr(args->a_files[0]);
		if(fhdr == NULL){
			LOG_ERROR("SHP_new_fhdr failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}

		if(SHP_read_fhdr(fp, fhdr)){
			LOG_ERROR("SHP_read_fhdr failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}

		SHP_dump_fhdr(stdout, fhdr);
		if(ftype == SFT_SHP){
			// TODO: fill this out!
		}else{
			int	i, n_recs;
			SF_RIDX_T	ridx;

			n_recs = (fhdr->sl_file - SF_FHDR_SIZE) / SF_RIDX_SIZE;
			for(i = 0; i < n_recs; i++){
				if(SHP_read_ridx(fp, &ridx)){
					LOG_ERROR("SHP_rad_ridx failed for record %d", i+1);
					err = 1;
					goto CLEAN_UP;
				}
				SHP_dump_ridx(stdout, &ridx);
			}
		}
		break;
	case SFT_DBF :
		// TODO: refactor.
		// 1. init a DBASE_T
		// 2. read the file header into a struct, 
		// 3. read the fieldinfo into a struct,
		// 4. read the current rec into a buf, then dump it.
		dbm = DBF_new_dbf_meta(args->a_files[0]);
		if(dbm == NULL){
			LOG_ERROR("DBF_new_dbf_meta failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
		if(DBF_read_dbf_meta(fp, dbm)){
			LOG_ERROR("DBF_read_dbf_meta failed for %s", args->a_files[0]);
			err = 1;
			goto CLEAN_UP;
		}
		DBF_dump_dbf_meta(stdout, dbm, verbose);
		break;
	case SFT_PRJ :
		break;
	default :
		LOG_ERROR("unkonwn file type %d", ftype);
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if(fp != NULL)
		fclose(fp);

	if(fhdr != NULL)
		SHP_delete_fhdr(fhdr);

	if(dbm != NULL)
		DBF_delete_dbf_meta(dbm);

	TJM_free_args(args);

	exit(err);
}
