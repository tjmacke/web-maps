#! /bin/bash
#
# input file: property file with the colors for the containing meighborhoods
#	header:	rnum, title, id, fill, all tab separated.
#
#	rnum:	shapefile record num
#	title:	shape name. For shapes belonging to a containing shape,
#		name is outer/inner.  For shapes without subshapes, name
#		is just outer without any slash
#	id:	name of of title's containing shape, which is outer/ if 
#		title conttain subshapes or just title if it doesn't. An
#		ending / indicates id contains subshapes
#	fill:	The fill value for this poly, or containing poly for
#		shapes with subshapes as #rrggbb
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] [ -fmt F ] -sf shape-file -id id-field-name [ prop-file ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src
WM_DATA=$WM_HOME/data
WM_SCRIPTS=$WM_HOME/scripts
WM_DEMOS=$WM_HOME/demos

TMP_BC_FILE=/tmp/2l.bc_colors.$$
TMP_ON_FILE=/tmp/2l.on_nabes.$$
TMP_IN_FILE=/tmp/2l.in_nabes.$$
TMP_CFILE=/tmp/2l.colors.$$

USE_BUILD=
FMT=
SFILE=
ID=
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
	-fmt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-fmt requires format argument, one of wrap, plain, list"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FMT=$1
		shift
		;;
	-sf)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sf requries shape-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SFILE=$1
		shift
		;;
	-id)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-id requires id-field-name argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		ID="$1"
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
	LOG ERROR "extra arguments $*"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
else
	BINDIR=$WM_BIN
fi

if [ ! -z "$FMT" ] ; then
	FMT="-fmt $FMT"
fi

if [ -z "$SFILE" ] ; then
	LOG ERROR "missing -sf shape-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$ID" ] ; then
	LOG ERROR "missing -id id-field-name argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

# The contents of the input will be used multiple times, so copy to temp to deal with stdin
cat $FILE > $TMP_BC_FILE
# 1. collect all outer level shapes, ie those with id ending with /
awk -F'\t' 'BEGIN {
	id = "'"$ID"'"
}
NR == 1 {
	f_id = 0
	for(i = 1; i <= NF; i++){
		if($i == id){
			f_id = i
			break
		}
	}
	if(f_id == 0){
		printf("ERROR: main: id field %s is not in header\n", id) > "/dev/stderr"
		err = 1
		exit err
	}
}
NR > 1 {
	if($f_id ~ /\/$/){
		if(!($f_id in id_tab)){
			n_idtab++
			id_tab[$f_id] = 1
		}
	}
}
END {
	n = asorti(id_tab)
	for(i = 1; i <= n; i++)
		printf("%s\n", id_tab[i])
}' $TMP_BC_FILE	> $TMP_ON_FILE
# 2. collect all inner shapes contained in each outer shape
#    extract them from the shape file
#    color them using varieties of the base color
cat $TMP_ON_FILE	|
while read line ; do
	awk -F'\t' 'BEGIN {
		id = "'"$ID"'"
		this_id = "'"$line"'"
	}
	NR == 1 {
		# will work b/c it worked in stage 1
		for(i = 1; i <= NF; i++){
			if($i == id){
				f_id = i
				break
			}
		}
		print $0
	}
	NR > 1 {
		if($f_id == this_id)
			print $0
	}' $TMP_BC_FILE	> $TMP_IN_FILE
	bcolor="$($WM_SCRIPTS/cval_to_cname.sh "$(tail -1 $TMP_IN_FILE | awk -F'\t' '{ print $NF }')")"
	tail -n +2 $TMP_IN_FILE							|
	$BINDIR/shp_to_geojson -sf $SFILE -pf $TMP_IN_FILE -pk rnum 		|
	$WM_SCRIPTS/find_adjacent_polys.sh -fmt wrapped -id title 		|
	$WM_SCRIPTS/color_graph.sh -bc $bcolor -id title			> $TMP_CFILE 
	# 3. update the original colors
	$WM_SCRIPTS/upd_column_values.sh -mk title -b $TMP_BC_FILE $TMP_CFILE 
done
# 4. return the a new pfile w/updated colors
cat $TMP_BC_FILE

rm -f $TMP_BC_FILE $TMP_ON_FILE $TMP_IN_FILE $TMP_CFILE
