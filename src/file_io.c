#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"

int
FIO_read_be_int4(FILE *fp, int *ival)
{
	int	i, c;
	int	err = 0;

	for(*ival = i = 0; i < 4; i++){
		if((c = getc(fp)) == EOF){
			LOG_ERROR("short integer: %d bytes, need 4", i);
			err = 1;
			goto CLEAN_UP;
		}
		*ival = (*ival << 8) | (c & 0xff);
	}

CLEAN_UP : ;

	if(err)
		*ival = 0;

	return err;
}

int
FIO_read_le_int4(FILE *fp, int *ival)
{
	int	i, c;
	int	err = 0;

	for(*ival = i = 0; i < 4; i++){
		if((c = getc(fp)) == EOF){
			LOG_ERROR("short integer: %d bytes, need 4", i);
			err = 1;
			goto CLEAN_UP;
		}
		*ival = *ival | ((c & 0xff) << (i * 8));
	}

CLEAN_UP : ;

	if(err)
		*ival = 0;

	return err;
}

int
FIO_read_le_double(FILE *fp, double *dval)
{
	int	i, c;
	union {
		char	v_str[8];
		double	v_double;
	} c2d;
	int	err = 0;

	*dval = 0;

	for(c2d.v_double = 0, i = 0; i < 8; i++){
		if((c = getc(fp)) == EOF){
			LOG_ERROR("short double: %d bytes, need 8", i);
			err = 1;
			goto CLEAN_UP;
		}
		c2d.v_str[i] = (c & 0xff);
	}
	*dval = c2d.v_double;

CLEAN_UP : ;

	if(err)
		*dval = 0;

	return err;
}
