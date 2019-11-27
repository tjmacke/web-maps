#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] -sel selector [ -json ] ] shape-file"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src

TMP_NFILE=/tmp/names.$$

USE_BUILD=
SEL=
BTYPE=shape
JSON=
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
	-sel)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sel requires selector argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SEL=$1
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
		SHP_FILE=$1
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

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
else
	BINDIR=$WM_BIN
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

$SEL $SHP_ROOT.db | tail -n +2 > $TMP_NFILE	# omit header

# this next section relies on the fact the -v dumps only bounding boxes.
$BINDIR/shp_dump -v $SHP_ROOT.shp 2> /dev/null	|
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
		# TODO: add code to get bbox for entire file?
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
