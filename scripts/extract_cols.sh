#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -hdr {keep*|drop|none} ] -c col-list [ prop-file ]"

HDR="keep"
CLIST=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-hdr)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-hdr header option"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		HDR=$1
		shift
		;;
	-c)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-c requires col-list argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		CLIST="$1"
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		FILE=$1
		shift
		break
		;;
	esac
done

if [ $# -ne 0 ] ; then
	LOG ERROR "extra arguments $*"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$CLIST" ] ; then
	LOG ERROR "missing -c col-list argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' 'BEGIN {
	hdr = "'"$HDR"'"
	if(hdr != "none")
		fix_hdr = 1;
	n_ftab = split("'"$CLIST"'", ftab, ",")
	for(i = 1; i <= n_ftab; i++){
		sub(/^  */, "", ftab[i])
		sub(/  *$/, "", ftab[i])
		if(hdr == "none"){
			# field names must be integers
			if(ftab[i] !~ /^[1-9][0-9]*$/){
				printf("ERROR: BEGIN: field %d: %s is not an integer\n", i, ftab[i]) > "/dev/stderr"
				err = 1
				exit err
			}
			f_idx[i] = i
		}else
			f_idx[ftab[i]] = ""
	}
}
{
	if(fix_hdr){
		fix_hdr = 0
		for(i = 0; i <= NF; i++){
			if($i in f_idx)
				f_idx[$i] = i
		}
		for(f in f_idx){
			if(f_idx[f] == ""){
				printf("ERROR: main: field %s is not in header\n", f) > "/dev/stderr"
				err = 1
			}
		}
		if(err)
			exit err
		if(hdr == "drop")
			next
		printf("%s", ftab[1])
		for(i = 2; i <= n_ftab; i++)
			printf("\t%s", ftab[i])
		printf("\n")
		next
	}
	printf("%s", $f_idx[ftab[1]])
	for(i = 2; i <= n_ftab; i++)
		printf("\t%s", $f_idx[ftab[i]])
	printf("\n")
}
END {
	if(err)
		exit err
	exit err
}' $FILE
