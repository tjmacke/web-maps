#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] [ -trace ] [ -sa adj-file ] [ -fmt F ] [ -h [ -d { lines*|colors|union } ] ] -sel selector shape-file"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src
WM_DATA=$WM_HOME/data
WM_SCRIPTS=$WM_HOME/scripts

TMP_PFILE=/tmp/pfile.$$
TMP_RNFILE=/tmp/rnums.$$
TMP_CFILE=/tmp/colors.$$
TMP_PFILE_2=/tmp/pfile_2.$$

USE_BUILD=
TRACE=
AFILE=
FMT=
HOPT=
HDISP=
SEL=
SHP_FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-b)
		USE_BUILD="yes"
		shift
		;;
	-trace)
		TRACE="yes"
		shift
		;;
	-sa)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sa requires adj-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		AFILE=$1
		shift
		;;
	-fmt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-fmt requires format arg, one of wrap, plain, list"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FMT=$1
		shift
		;;
	-h)
		HOPT="yes"
		shift
		;;
	-d)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-d requires display argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		HDISP=$1
		shift
		;;
	-sel)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sel requires selector argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SEL="$1"
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		SHP_FILE=$1
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

if [ -z "$SEL" ] ; then
	LOG ERROR "missing -sel selector argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$SHP_FILE" ] ; then
	LOG ERROR "missing shape-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi
# TODO: do this right
SHP_ROOT="$(echo $SHP_FILE | awk -F'.' '{ r = $1 ; for(i = 2; i < NF; i++) { r = r "." $i } ; print r }')"

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
	BOPT="-b"
else
	BINDIR=$WM_BIN
	BOPT=
fi

if [ ! -z "$FMT" ] ; then
	FMT="-fmt $FMT"
fi

if [ "$HOPT" == "yes" ] ; then
	ID="id"
	MK="id"
	if [ -z "$HDISP" ] ; then
		HDISP="lines"
	elif [ "$HDISP" == "union" ] ; then
		LOG ERROR "-d union not yet implemented, use lines or colors"
		exit 1
	elif [ "$HDISP" != "lines" ] && [ "$HDISP" != "colors" ] ; then
		LOG ERROR "bad hierarchy display type $HDISP, must be one of lines, colors or union"
		echo "$U_MSG" 1>&2
		exit 1
	fi
	HOPT="-h"
elif [ ! -z "$HDISP" ] ; then
	LOG ERROR "-d requires -h option"
	echo "$U_MSG" 1>&2
	exit 1
else
	ID="title"
	MK="title"
fi

if [ ! -z "$TRACE" ] ; then
	TRACE="-trace"
fi

$SEL $HOPT $SHP_ROOT.db > $TMP_PFILE
tail -n +2 $TMP_PFILE | awk '{ print $1 }'					> $TMP_RNFILE
$BINDIR/shp_to_geojson -sf $SHP_ROOT -pf $TMP_PFILE -pk rnum $TMP_RNFILE	|
$WM_SCRIPTS/find_adjacent_polys.sh $TRACE -fmt wrapped -id $ID			|
$WM_SCRIPTS/rm_dup_islands.sh							|
if [ ! -z "$AFILE" ] ; then
	tee $AFILE
else
	cat
fi										|
$WM_SCRIPTS/color_graph.sh $TRACE -id $ID					> $TMP_CFILE
if [ "$HDISP" == "colors" ] ; then
	$WM_SCRIPTS/add_columns.sh -b $TMP_PFILE -mk $MK $TMP_CFILE		|
	$WM_SCRIPTS/make_2l_colors.sh $BOPT $FMT -sf $SHP_ROOT -id id		> $TMP_PFILE_2
else
	$WM_SCRIPTS/add_columns.sh -b $TMP_PFILE -mk $MK $TMP_CFILE		>  $TMP_PFILE_2
fi
$BINDIR/shp_to_geojson -sf $SHP_ROOT -pf $TMP_PFILE_2 -pk rnum $FMT $TMP_RNFILE

rm -f $TMP_PFILE $TMP_RNFILE $TMP_CFILE $TMP_PFILE_2
