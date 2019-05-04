#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -bt { shape* | outer | both } [ -json ] ] shp-file"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin

TMP_NFILE=/tmp/names.$$

function mk_fname() {
	echo "$(echo $1 $2	|
	awk '{
		n_ary = split($1, ary, ".")
		if(ary[nf] == $2)
			print $1
		else{
			fn = ary[1]
			for(i = 2; i < n_ary; i++)
				fn = fn "." ary[i]
			fn = fn "." $2
			print fn
		}
	}')"
}

BTYPE=shape
JSON=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-bt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-bt requires bbox type"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		BTYPE=$1
		shift
		;;
	-json)
		JSON="yes"
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
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
	LOG ERROR "extra arguments: $*"
	echo "$U_MSG" 1>&2
	exit 1
fi

G_OUTER=
G_SHAPE=
if [ ! -z "$BTYPE" ] ; then
	if [ "$BTYPE" == "outer" ] ; then
		G_OUTER="yes"
	elif [ "$BTYPE" == "shape" ] ; then
		G_SHAPE="yes"
	elif [ "$BTYPE" == "both" ] ; then
		G_OUTER="yes"
		G_SHAPE="yes"
	else
		LOG ERROR "unknown bbox type $BTYPE, must be shape, outer or both"
		exit 1
	fi
else
	G_SHAPE="yes"
fi

if [ -z "$FILE" ] ; then
	LOG ERROR "missing shp-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi
SHP_FILE=$(mk_fname $FILE "shp")

if [ "$G_SHAPE" == "yes" ] ; then
	DB_FILE=$(mk_fname $FILE "db")
	sqlite3 $DB_FILE <<-_EOF_ |
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
			if(title == "")
				continue
			t_count[title]++
		}
		printf("%s\t%s\n", "rnum", "title")
		for(i = 1; i <= n_recs; i++){
			title = titles[i]
			if(title == "")
				continue
			n_ary = split(recs[i], ary)
			if(t_count[title] > 1){
				if(title != /\/$/)
					title = title "_" ary[fnums["OBJECTID"]]
			}
			printf("%s\t%s\n", ary[fnums["OBJECTID"]], title)
		}
	}
	function mk_title(fnums, rec,   n_ary, ary, i, title) {

		n_ary = split(rec, ary)
		if(ary[fnums["L_HOOD"]] == "")
			title = ""
		else if(ary[fnums["L_HOOD"]] == "NO BROADER TERM")
			title = ary[fnums["S_HOOD"]]
		else
			title = sprintf("%s/%s", ary[fnums["L_HOOD"]], ary[fnums["S_HOOD"]])
		return title
	}' > $TMP_NFILE
fi
if [ "$G_OUTER" == "yes" ] ; then
	echo -e "-1\t_outer_" >> $TMP_NFILE
fi

# this next section relies on the fact the -v dumps only bounding boxes.
$WM_BIN/shp_dump -v $SHP_FILE	|
awk 'BEGIN {
	json = "'"$JSON"'" == "yes"
	nfile = "'"$TMP_NFILE"'"
	for(n_ntab = 0; (getline line < nfile) > 0; ){
		n_ary = split(line, ary, "\t")
		n_ntab++
		ntab[ary[1]] = ary[2]
	}
	close(nfile)
	first = 1
}
{
	if($1 == "fhdr")
		rnum = -1
	else if($1 == "rnum")
		rnum = $3
	else if($1 == "xmin")
		xmin = $3
	else if($1 == "ymin")
		ymin = $3
	else if($1 == "xmax")
		xmax = $3
	else if($1 == "ymax"){
		ymax = $3
		pr = 1
	}
	if(pr){
		pr = 0
		if(rnum in ntab){
			if(json)
				first = mk_geojson(first, rnum, ntab, xmin, ymin, xmax, ymax)
			else
				printf("%s\t%s\t%s\t%s\t%s\n", ntab[rnum], xmin, ymin, xmax, ymax)
		}
		# get outer "_outer_" block only
		if(rnum == -1 && n_ntab == 1)
			exit 0
	}
}
END {
	if(!first){
		printf(" ]\n")
		printf("}\n")
		printf("}\n")
	}
}
function mk_geojson(first, rnum, ntab, xmin, ymin, xmax, ymax) {

	if(first){
		first = 0
		printf("{\n")
		printf("\"geojson\": {\n")
		printf("  \"type\": \"FeatureCollection\",\n")
		printf("  \"features\": [\n")
	}else
		printf(",\n")
	mk_bbox(rnum, ntab, xmin, ymin, xmax, ymax)
	return first
}
function mk_bbox(rnum, ntab, xmin, ymin, xmax, ymax) {

	printf("{\n")
	printf("  \"type\": \"Feature\",\n")
	printf("  \"geometry\": {\n")
	printf("    \"type\": \"Polygon\",\n")
	printf("    \"coordinates\": [\n")
	printf("[\n")
	printf("[%s, %s],\n", xmin, ymin)
	printf("[%s, %s],\n", xmax, ymin)
	printf("[%s, %s],\n", xmax, ymax)
	printf("[%s, %s],\n", xmin, ymax)
	printf("[%s, %s]\n", xmin, ymin)
	printf("]\n")
	printf("    ]\n")
	printf("  },\n")
	title = rnum != -1 ? ntab[rnum] : "_outer_"
	sub(/\//, "\\/", title)
	printf("  \"properties\": {\"rnum\": %d, \"title\": \"%s\"}\n", rnum, title)
	printf("}")
	return 0
}'

rm -f $TMP_NFILE
