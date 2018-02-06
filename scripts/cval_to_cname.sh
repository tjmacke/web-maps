#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0: [ -help ] color-value"

WM_SCRIPTS=$WM_HOME/scripts

# awk v3 does not support includes
AWK_VERSION="$(awk --version | awk '{ nf = split($3, ary, /[,.]/) ; print ary[1] ; exit 0 }')"
if [ "$AWK_VERSION" == "3" ] ; then
	AWK="igawk --re-interval"
	COLOR_DATA="$WM_SCRIPTS/color_data.awk"
elif [ "$AWK_VERSION" == "4" ] ; then
	AWK=awk
	COLOR_DATA="\"$WM_SCRIPTS/color_data.awk\""
else
	LOG ERROR "unsupported awk version: \"$AWK_VERSION\": must be 3 or 4"
	exit 1
fi

CVAL=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		CVAL="$1"
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

if [ -z "$CVAL" ] ; then
	LOG ERROR "missing color-value argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

echo "$CVAL"	|\
$AWK -F'\t' '
@include '"$COLOR_DATA"'
{
	if($1 in cval_to_cname)
		printf("%s\n", cval_to_cname[$1])
	else
		printf("_UNKNOWN_CVAL_\n")
}'
