#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] [ -sa adj-file ] (no arguments)"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src
WM_DATA=$WM_HOME/data
WM_SCRIPTS=$WM_HOME/scripts
WA_DATA=$WM_DATA/cb_2016_place_500k

TMP_PFILE=/tmp/53.tsv.$$
TMP_RNFILE=/tmp/53.rnums.$$
TMP_CFILE=/tmp/53.colors.tsv.$$
TMP_PFILE_2=/tmp/53_2.tsv.$$

USE_BUILD=
AFILE=

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
	-*)
		LOG_ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		LOG_ERROR "extra arguments $*"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	esac
done

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
else
	BINDIR=$WM_BIN
fi

# 1. select ALL places in Washington state
sqlite3 $WA_DATA/cb_2016_53_place_500k.db <<_EOF_ > $TMP_PFILE
.headers on
.mode tabs
select rnum, NAME || ', ' || statefp.STUSAB || '_' || PLACEFP as title from data, statefp where data.STATEFP = statefp.STATEFP ;
_EOF_
tail -n +2 $TMP_PFILE | awk '{ print $1 }'							> $TMP_RNFILE
$BINDIR/shp_to_geojson -sf $WA_DATA/cb_2016_53_place_500k -pf $TMP_PFILE -pk rnum $TMP_RNFILE	|\
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped -id title					|\
$WM_SCRIPTS/rm_dup_islands.sh									|\
if [ ! -z "$AFILE" ] ; then
	tee $AFILE
else
	cat
fi												|\
$WM_SCRIPTS/color_graph.sh -id title								> $TMP_CFILE
$WM_SCRIPTS/add_columns.sh -mk title -b $TMP_PFILE $TMP_CFILE 					> $TMP_PFILE_2
$BINDIR/shp_to_geojson -sf $WA_DATA/cb_2016_53_place_500k -pf $TMP_PFILE_2 -pk rnum $TMP_RNFILE

rm -f $TMP_PFILE $TMP_RNFILE $TMP_CFILE $TMP_PFILE_2
