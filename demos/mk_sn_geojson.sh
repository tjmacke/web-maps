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
SND_HOME=$WM_DATA/Seattle_Neighborhoods
SND_DATA=$SND_HOME/WGS84

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
}' > sn.tsv

tail -n +2 sn.tsv | awk '{ print $1 }'							> sn.rnums
$BINDIR/shp_to_geojson -sf $SND_DATA/Neighborhoods -pf sn.tsv -pk rnum	sn.rnums	|\
$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped						|\
$WM_SCRIPTS/color_graph.sh 								> sn.colors
$WM_SCRIPTS/add_columns.sh -mk title sn.tsv sn.colors 					> sn.all.tsv
$BINDIR/shp_to_geojson -sf $SND_DATA/Neighborhoods -pf sn.all.tsv -pk rnum sn.rnums
