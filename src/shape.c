#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "shape.h"

int
SHP_get_file_type(const char *fname)
{
	int	ftype = SFT_UNKNOWN;
	const char	*fnp;
	const char	*slash, *dot;
	int	err = 0;

	if(fname == NULL || *fname == '\0'){
		LOG_ERROR("fname is NULL or empty");
		err = 1;
		goto CLEAN_UP;
	}

	for(slash = dot = NULL, fnp = fname; *fnp; fnp++){
		if(*fnp == '/')
			slash = fnp;
		else if(*fnp == '.')
			dot = fnp;
	}

	// find the extension
	if(dot == NULL){
		LOG_ERROR("no extension: %s", fname);
		err = 1;
		goto CLEAN_UP;
	}else if(slash){
		if(dot < slash){	 // last dot is a directory
			LOG_ERROR("no extension: %s", fname);
			err = 1;
			goto CLEAN_UP;
		}
	}
	dot++;
	if(!strcmp(dot, "shp"))
		ftype = SFT_SHP;
	else if(!strcmp(dot, "shx"))
		ftype = SFT_SHX;
	else if(!strcmp(dot, "dbf"))
		ftype =  SFT_DBF;
	else if(!strcmp(dot, "prj"))
		ftype = SFT_PRJ;
	else{
		LOG_ERROR("unknown file type '%s'", dot);
		err = 1;
		goto CLEAN_UP;
	}


CLEAN_UP : ;

	if(err)
		ftype = SFT_UNKNOWN;

	return ftype;
}
