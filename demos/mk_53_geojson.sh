#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BUILD=$WM_HOME/src
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data
WM_SCRIPTS=$WM_HOME/scripts
WA_DATA=$WM_DATA/tl_2017_place

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

# 1. select the neighborhoods. Currenty neighborhoods w/L_HOOD = "" are skipped
sqlite3 $WA_DATA/tl_2017_53_place.db <<_EOF_ > 53.tsv
.headers on
.mode tabs
select rnum, NAME || ', ' || statefp.STUSAB || '_' || PLACEFP as title from data, statefp where data.STATEFP = statefp.STATEFP ;
_EOF_
tail -n +2 53.tsv | awk '{ print $1 }'							> 53.rnums
$BINDIR/shp_to_geojson -sf $WA_DATA/tl_2017_53_place -pf 53.tsv -pk rnum 53.rnums	|\
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped						|\
./rm_dup_islands.sh									|\
$WM_SCRIPTS/color_graph.sh 								> 53.colors.tsv
$WM_SCRIPTS/add_columns.sh -mk title 53.tsv 53.colors.tsv 				> 53.all.tsv
$BINDIR/shp_to_geojson -sf $WA_DATA/tl_2017_53_place -pf 53.all.tsv -pk rnum 53.rnums
