#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <csv.h>

#include "log.h"
#include "args.h"

#define	S_BUF	1024
#define	BOM	"\357\273\277"
#define	l_BOM	3

static	ARGS_T	*args;
static	FLAG_T	flags[] = {
	{"-help", 1, AVK_NONE, AVT_BOOL, "0",  "Use -help to print this message."},
	{"-v",    1, AVK_OPT,  AVT_UINT, "0",  "Use -v to set the verbosity to 1; use -v=N to set it to N."},
	{"-bom",  1, AVK_NONE, AVT_BOOL, "0",  "Use -bom to check for and remove a Byte Order Mark."},
	{"-dot",  1, AVK_NONE, AVT_BOOL, "0",  "Use -dot to print \".\" for empty fields."},
	{"-ch",   1, AVK_REQ,  AVT_STR,  NULL, "Use -ch F to write the column headers to file F."}
};
static	int	n_flags = sizeof(flags)/sizeof(flags[0]);

typedef	struct	c2t_state_t	{
	int	c_bom;
	int	c_dot;
	FILE	*c_chfp;
	int	cn_fields_r1;
	int	cn_fields;
	int	cn_rows;
} C2T_STATE_T;

static	void
C2T_pr_string(FILE *fp, const char *str, size_t s_len)
{
	const char	*sp;
	size_t	i;
	int	c;

	// revisit this
	for(sp = str, i = 0; i < s_len; i++){
		c = *sp++ & 0xff;;
		switch(c){
		case '\\' :
			putc('\\', fp);
			putc('\\', fp);
			break;
		case '\n' :
			putc('\\', fp);
			putc('n', fp);
			break;
		case '\r' :
			putc('\\', fp);
			putc('r', fp);
			break;
		case '\t' :
			putc('\\', fp);
			putc('t', fp);
			break;
		default :
			putc(c, fp);
			break;
		}
	}
}

static	void
C2T_pr_field(FILE *fp, C2T_STATE_T *cp, const char *str, size_t s_len)
{
	char	*sp = (char *)str;

}

void
cb_efld(void *s, size_t len, void *data)
{
	C2T_STATE_T	*cp = (C2T_STATE_T *)data;
	const char	*sp = (const char *)s;
	size_t		s_len = len;

	cp->cn_fields++;

	// remove the BOM?
	if(cp->cn_fields == 1 && cp->cn_rows == 0 && cp->c_bom){
		if(!strncmp(sp, BOM, l_BOM)){
			LOG_INFO("remove the BOM");
			sp += l_BOM;
			s_len -= l_BOM;
		}
	}

	// handle empty fields
	if(s_len == 0){
		if(cp->c_dot){
			sp = ".";
			s_len = 1;
		}
	}

	// print the field
	if(cp->cn_fields > 1)
		putc('\t', stdout);
	C2T_pr_string(stdout, sp, s_len);

	// print the header file
	if(cp->c_chfp != NULL){
		fprintf(cp->c_chfp, "%d\t", cp->cn_fields);
		C2T_pr_string(cp->c_chfp, sp, s_len);
		fprintf(cp->c_chfp, "\n");
	}
}

void
cb_erow(int c, void *data)
{
	C2T_STATE_T	*cp = (C2T_STATE_T *)data;
	int	i;

	cp->cn_rows++;
	if(cp->c_chfp != NULL){
		fclose(cp->c_chfp);
		cp->c_chfp = NULL;
	}
	if(cp->cn_rows == 1)
		cp->cn_fields_r1 = cp->cn_fields;
	else if(cp->cn_fields != cp->cn_fields_r1){
		// complain?
	}
	printf("\n");
	cp->cn_fields = 0;
}

static	C2T_STATE_T	c2t;
static	char	*buf;

int
main(int argc, char *argv[])
{
	int	a_stat = AS_OK;
	const ARG_VAL_T	*a_val;
	int	verbose = 0;
	int	bom = 0;
	int	dot = 0;
	const char	*ch_fname = NULL;
	FILE	*fp = NULL;

	// stuff used by libcsv
	struct csv_parser p;
	int	p_init = 0;
	int	s_buf;
	int	n_buf;
	size_t	bytes_read;

	int	err = 0;

	a_stat = TJM_get_args(argc, argv, n_flags, flags, 0, 1, &args);
	if(a_stat != AS_OK){
		err = a_stat != AS_HELP_ONLY;
		goto CLEAN_UP;
	}

	a_val = TJM_get_flag_value(args, "-v", AVT_UINT);
	verbose = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-bom", AVT_BOOL);
	c2t.c_bom = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-dot", AVT_BOOL);
	c2t.c_dot = a_val->av_value.v_int;

	a_val = TJM_get_flag_value(args, "-ch", AVT_STR);
	ch_fname = a_val->av_value.v_str;

	if(verbose > 1)
		TJM_dump_args(stderr, args);

	if(args->an_files == 0)
		fp = stdin;
	else if((fp = fopen(args->a_files[0], "r")) == NULL){
		LOG_ERROR("can't read input csv file %s", args->a_files[0]);
		err = 1;
		goto CLEAN_UP;
	}

	if(ch_fname != NULL){
		FILE	*ch_fp = NULL;

		if((ch_fp = fopen(ch_fname, "w")) == NULL){
			LOG_ERROR("can't write column header file %s", ch_fname);
			err = 1;
			goto CLEAN_UP;
		}
		c2t.c_chfp = ch_fp;
	}

	s_buf = S_BUF;
	buf = (char *)malloc(s_buf * sizeof(char));
	if(buf == NULL){
		LOG_ERROR("can't allocate buf");
		err = 1;
		goto CLEAN_UP;
	}

	if(csv_init(&p, 0) != 0){
		LOG_ERROR("csv_init() failed");
		err = 1;
		goto CLEAN_UP;
	}else
		p_init = 1;

	while((n_buf = fread(buf, 1, s_buf, fp)) > 0){
		if(csv_parse(&p, buf, n_buf, cb_efld, cb_erow, &c2t) != n_buf){
			LOG_ERROR("Error while parsing file: %s\n", csv_strerror(csv_error(&p)));
			err = 1;
			goto CLEAN_UP;
		}
	}

CLEAN_UP : ;

	if(p_init){
		csv_fini(&p, cb_efld, cb_erow, &c2t);
		csv_free(&p);
	}

	if(c2t.c_chfp != NULL)
		fclose(c2t.c_chfp);

	if(fp != NULL && fp != stdin)
		fclose(fp);

	TJM_free_args(args);

	exit(err);
}
