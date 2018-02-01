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
SND_DATA=$WM_DATA/Seattle_Neighborhoods/WGS84

TMP_PFILE=/tmp/sn.tsv.$$
TMP_RNFILE=/tmp/sn.rnums.$$
TMP_CFILE=/tmp/sn.colors.tsv.$$
TMP_PFILE_2=/tmp/sn_2.tsv.$$

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
		LOG_ERROR "unknown option $1"
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

# 1. select the neighborhoods. Currenty neighborhoods w/L_HOOD = "" are skipped
sqlite3 $SND_DATA/Neighborhoods.db <<_EOF_ |
.headers on
.mode tabs
select OBJECTID, L_HOOD, S_HOOD from data order by L_HOOD, S_HOOD ;
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
	for(i = 1; i <= n_recs; i++){
		titles[i] = title = mk_title(fnums, recs[i])
 		t_count[title]++
		if(title == "")
			continue;
	}
	printf("%s\t%s\n", "rnum", "title")
	for(i = 1; i <= n_recs; i++){
		title = titles[i]
		if(title == "")
			continue
		nf = split(recs[i], ary)
		if(t_count[title] > 1)
			title = title "_" ary[fnums["OBJECTID"]]
		printf("%s\t%s\n", ary[fnums["OBJECTID"]], title)
	}
}
function mk_title(fnums, rec, t_count,   nf, ary, i, title) {

	nf = split(rec, ary)
	if(ary[fnums["L_HOOD"]] == "")
		return ""
	if(ary[fnums["L_HOOD"]] == "NO BROADER TERM")
		title = ary[fnums["S_HOOD"]]
	else
		title = sprintf("%s/%s", ary[fnums["L_HOOD"]], ary[fnums["S_HOOD"]])
	t_count[title]++
	return title
}' > $TMP_PFILE

tail -n +2 $TMP_PFILE | awk '{ print $1 }'						> $TMP_RNFILE
$WM_BIN/shp_to_geojson -sf $SND_DATA/Neighborhoods -pf $TMP_PFILE -pk rnum $TMP_RNFILE	|\
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped						|\
$WM_SCRIPTS/rm_dup_islands.sh								|\
if [ ! -z "$AFILE" ] ; then
	tee $AFILE
else
	cat
fi											|\
$WM_SCRIPTS/color_graph.sh 								> $TMP_CFILE
$WM_SCRIPTS/add_columns.sh -mk title $TMP_PFILE $TMP_CFILE 				> $TMP_PFILE_2
$WM_BIN/shp_to_geojson -sf $SND_DATA/Neighborhoods -pf $TMP_PFILE_2 -pk rnum $TMP_RNFILE

rm -f $TMP_PFILE $TMP_RNFILE $TMP_CFILE $TMP_PFILE_2