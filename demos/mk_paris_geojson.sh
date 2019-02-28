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
PAR_DATA=$WM_DATA/Paris

TMP_PFILE=/tmp/par.tsv.$$
TMP_RNFILE=/tmp/par.rnums.$$
TMP_CFILE=/tmp/par.colors.tsv.$$
TMP_PFILE_2=/tmp/par_2.tsv.$$

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

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
else
	BINDIR=$WM_BIN
fi

# 1. select the data
sqlite3 $PAR_DATA/arrondissements.db <<_EOF_ |
.headers on
.mode tab
select rnum, l_ar as ar_num, l_aroff as ar_name from data ;
_EOF_
awk -F'\t' 'NR == 1 {
	for(i = 1; i <= NF; i++)
		fnums[$i] = i
}
NR > 1 {
	n_recs++
	recs[n_recs] = $0
}
END {
	printf("%s\t%s\n", "rnum", "title")
	for(i = 1; i <= n_recs; i++)
		printf("%d\t%s\n", i, mk_title(fnums, recs[i]))
}
function mk_title(fnums, rec,   n_ary, ary, n_ary2, ary2) {
	n_ary = split(rec, ary)
	n_ary2 = split(ary[fnums["ar_num"]], ary2, /  */)
	return sprintf("%-6s %s", (ary2[1] ":"), ary[fnums["ar_name"]])
}' > $TMP_PFILE
tail -n +2 $TMP_PFILE | awk '{ print $1 }' > $TMP_RNFILE
$BINDIR/shp_to_geojson -sf $PAR_DATA/arrondissements -pf $TMP_PFILE -pk rnum $TMP_RNFILE	|
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped -id title					|
$WM_SCRIPTS/rm_dup_islands.sh									|
if [ ! -z "$AFILE" ] ; then
	tee $AFILE
else
	cat
fi												|
$WM_SCRIPTS/color_graph.sh -id title								> $TMP_CFILE
$WM_SCRIPTS/add_columns.sh -mk title $TMP_PFILE $TMP_CFILE					> $TMP_PFILE_2
$BINDIR/shp_to_geojson -sf $PAR_DATA/arrondissements -pf $TMP_PFILE_2 -pk rnum $TMP_RNFILE
rm -rf $TMP_PFILE $TMP_RNFILE $TMP_CFILE $TMP_PFILE_2
