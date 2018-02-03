#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -sa adj-file ] (no arguments)"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data
WM_SCRIPTS=$WM_HOME/scripts
DD_DATA=$WM_DATA/cb_2016_place_500k/cb_2016_06_place_500k

TMP_PFILE=/tmp/dd.tsv.$$
TMP_RNFILE=/tmp/dd.rnums.$$
TMP_CFILE=/tmp/dd.colors.tsv.$$
TMP_PFILE_2=/tmp/dd_2.tsv.$$

AFILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
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
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		LOG ERROR "extra arguments $*"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	esac
done

cat <<_EOF_ > $TMP_PFILE
title	rnum
Atherton, CA_03092	451
Belmont, CA_05108	345
Cupertino, CA_17610	1455
East Palo Alto, CA_20956	722
Emerald Lake Hills, CA_22587	974
Foster City, CA_25338	350
Ladera, CA_39094	72
Los Altos, CA_43280	300
Los Altos Hills, CA_43294	420
West Menlo Park, CA_84536	88
Menlo Park, CA_46870	723
Mountain View, CA_49670	388
Palo Alto, CA_55282	758
Portola Valley, CA_58380	724
Redwood City, CA_60102	178
San Carlos, CA_65070	321
Santa Clara, CA_69084	761
Stanford, CA_73906	947
Sunnyvale, CA_77000	305
Woodside, CA_86440	728
_EOF_
tail -n +2 $TMP_PFILE	| awk -F'\t' '{ print $NF }' 			> $TMP_RNFILE
$WM_BIN/shp_to_geojson -pf $TMP_PFILE -pk rnum -sf $DD_DATA $TMP_RNFILE	|\
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped -id title 		|\
$WM_SCRIPTS/rm_dup_islands.sh						|\
if [ ! -z "$AFILE" ] ; then
	tee $AFILE
else
	cat
fi	|\
$WM_SCRIPTS/color_graph.sh						> $TMP_CFILE
$WM_SCRIPTS/add_columns.sh -mk title $TMP_PFILE $TMP_CFILE 		> $TMP_PFILE_2
$WM_BIN/shp_to_geojson -pf $TMP_PFILE_2 -pk rnum -sf $DD_DATA $TMP_RNFILE

rm -f $TMP_PFILE $TMP_RNFILE $TMP_CFILE $TMP_PFILE_2
