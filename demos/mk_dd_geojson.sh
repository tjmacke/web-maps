#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] -i index-file [ place-names-file ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BUILD=$WM_HOME/src
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data
WM_SCRIPTS=$WM_HOME/scripts
DD_DATA=$WM_HOME/data/tl_2017_place/tl_2017_06_place

USE_BUILD=
IFILE=
FILE=

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
	-i)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-i requires index-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		IFILE=$1
		shift
		;;
	-*)
		LOG ERROR "unknown argument $1"
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

if [ -z "$IFILE" ] ; then
	LOG ERROR "missing -i index-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
else
	BINDIR=$WM_BIN
fi

rm -f dd.tsv dd.rnums dd.adj.tsv dd.colors.tsv dd.all.tsv
echo -e "title\tLSASD\tSTATEFP\trnum" > dd.tsv
cat $FILE	|\
while read line ; do
	grep "$line" $IFILE
done	| sort -u	>> dd.tsv
tail -n +2 dd.tsv	| awk -F'\t' '{ print $NF }' 			> dd.rnums
$BINDIR/shp_to_geojson -pf dd.tsv -pk rnum -sf $DD_DATA dd.rnums	|\
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped 			|\
$WM_SCRIPTS/color_graph.sh						> dd.colors.tsv
$WM_SCRIPTS/add_columns.sh -mk title dd.tsv dd.colors.tsv 		> dd.all.tsv
$BINDIR/shp_to_geojson -pf dd.all.tsv -pk rnum -sf $DD_DATA dd.rnums
