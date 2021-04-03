#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -k key [ -fmt { 12b|24b* } ] [ prop-file ]"

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOE is not defined"
	exit 1
fi
DM_ETC=$DM_HOME/etc
DM_LIB=$DM_HOME/lib
DM_SCRIPTS=$DM_HOME/scripts

# awk v3 does not support include
AWK_VERSION="$(awk --version | awk '{ nf = split($3, ary, /[,.]/) ; print ary[1] ; exit 0 }')"
if [ "$AWK_VERSION" == "3" ] ; then
	AWK="igawk --re-interval"
	COLOR_UTILS="$DM_LIB/color_utils.awk"
elif [ "$AWK_VERSION" == "4" ] || [ "$AWK_VERSION" == "5" ] ; then
	AWK=awk
	COLOR_UTILS="\"$DM_LIB/color_utils.awk\""
else
	LOG ERROR "unsupported awk version: \"$AWK_VERSION\": must be 3, 4 or 5"
	exit 1
fi

KEY=
FMT=24b
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-k)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-k requires key argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		KEY=$1
		shift
		;;
	-fmt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-fmt requires format argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FMT=$1
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

if [ -z "$KEY" ] ; then
	LOG ERROR "missing -k key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ "$FMT" != "12b" ] && [ "$FMT" != "24b" ] ; then
	LOG ERROR "unknown fmt $FMT, must be 12b or 24b"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' '
@include '"$COLOR_UTILS"'
BEGIN {
	key = "'"$KEY"'"
	fmt = "'"$FMT"'"
}
NR == 1 {
	f_key = -1
	for(i = 1; i <= NF; i++){
		if($i == key){
			f_key = i
			break
		}
	}
	if(f_key == -1){
		printf("ERROR: BEGIN: key %s not in header\n", key) > "/dev/stderr"
		err = 1
		exit err
	}
	printf("%s\n", $0)
}
NR > 1 {
	for(i = 1; i < f_key; i++)
		printf("%s%s", i > 1 ? "\t" : "", $i)
	printf("%s#%s", f_key > 1 ? "\t" : "", fmt == "12b" ? CU_rgb_to_12bit_color($f_key) : CU_rgb_to_24bit_color($f_key))
	for(i = f_key + 1; i <= NF; i++)
		printf("\t%s", $i)
	printf("\n")
}' $FILE
