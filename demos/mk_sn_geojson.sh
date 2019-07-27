#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] [ -trace ] [ -sa adj-file ] [ -fmt F ] [ -h [ -d { lines*|colors|union } ] ] shape-file"

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

if [ -z "$SHP_FILE" ] ; then
	LOG ERROR "missing shape-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi
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
	PFX="-pfx_of title"
	SAVE_BC=
	if [ -z "$HDISP" ] ; then
		HDISP="lines"
	elif [ "$HDISP" == "union" ] ; then
		LOG ERROR "-d union not yet implemented, use lines or colors"
		exit 1
	elif [ "$HDISP" == "colors" ] ; then
		SAVE_BC="yes"
	elif [ "$HDISP" != "lines" ] ; then
		LOG ERROR "bad hierarchy display type $HDISP, must be one of union, lines or colors"
		echo "$U_MSG" 1>&2
		exit 1
	fi
elif [ ! -z "$HDISP" ] ; then
	LOG ERROR "-d requires -h option"
	echo "$U_MSG" 1>&2
	exit 1
else
	ID="title"
	MK="title"
	PFX=
fi

if [ ! -z "$TRACE" ] ; then
	TRACE="-trace"
fi

# 1. select the shapes
sqlite3 $SHP_ROOT.db <<_EOF_	|
.headers on
.mode tabs
select OBJECTID, L_HOOD, S_HOOD from data order by L_HOOD, S_HOOD ;
_EOF_
awk -F'\t' 'BEGIN {
	hopt = "'"$HOPT"'" == "yes"
	hdisp = "'"$HDISP"'"
}
NR == 1 {
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
		if(title == "")
			continue;
 		t_count[title]++
		if(hopt){
			ids[i] = id = mk_id(fnums, recs[i])
			i_count[id]++
		}
	}
	printf("%s\t%s", "rnum", "title")
	if(hopt)
		printf("\t%s", "id")
	printf("\n")
	for(i = 1; i <= n_recs; i++){
		title = titles[i]
		if(title == "")
			continue
		nf = split(recs[i], ary)
		if(t_count[title] > 1){
			if(title !~ /\/$/)
				title = title "_" ary[fnums["OBJECTID"]]
		}
		printf("%s\t%s", ary[fnums["OBJECTID"]], title)
		if(hopt){
			id = mk_id(fnums, recs[i])
			if(i_count[id] > 1){
				if(id !~ /\/$/)
					id = id "_" ary[fnums["OBJECTID"]]
			}
			printf("\t%s", id)
		}
		printf("\n")
	}
}
function mk_title(fnums, rec,   nf, ary, i, title) {

	nf = split(rec, ary)
	if(ary[fnums["L_HOOD"]] == "")
		title = ""
	else if(ary[fnums["L_HOOD"]] == "NO BROADER TERM")
		title = ary[fnums["S_HOOD"]]
	else
		title = sprintf("%s/%s", ary[fnums["L_HOOD"]], ary[fnums["S_HOOD"]])
	return title
}
function mk_id(fnums, rec,   nf, ary, i, id) {

	nf = split(rec, ary)
	if(ary[fnums["L_HOOD"]] == "")
		id = ""
	else if(ary[fnums["L_HOOD"]] == "NO BROADER TERM")
		id = ary[fnums["S_HOOD"]] 
	else
		id = sprintf("%s/", ary[fnums["L_HOOD"]])
	return id
}' > $TMP_PFILE
tail -n +2 $TMP_PFILE | awk '{ print $1 }'					> $TMP_RNFILE
$BINDIR/shp_to_geojson -sf $SHP_ROOT -pf $TMP_PFILE -pk rnum $TMP_RNFILE	|\
$WM_SCRIPTS/find_adjacent_polys.sh $TRACE -fmt wrapped -id $ID			|\
$WM_SCRIPTS/rm_dup_islands.sh							|\
if [ ! -z "$AFILE" ] ; then
	tee $AFILE
else
	cat
fi										|\
$WM_SCRIPTS/color_graph.sh $TRACE -id $ID					> $TMP_CFILE
if [ -z "$SAVE_BC" ] ; then
	$WM_SCRIPTS/add_columns.sh $TRACE -mk $MK $PFX $TMP_PFILE $TMP_CFILE	> $TMP_PFILE_2
	$BINDIR/shp_to_geojson -sf $SHP_ROOT -pf $TMP_PFILE_2 -pk rnum $FMT $TMP_RNFILE
else
	$WM_SCRIPTS/add_columns.sh $TRACE -mk $MK $PFX $TMP_PFILE $TMP_CFILE	|\
	$WM_SCRIPTS/make_2l_colors.sh $BOPT $FMT -sf $SHP_ROOT -id id
fi

rm -f $TMP_PFILE $TMP_RNFILE $TMP_CFILE $TMP_PFILE_2
