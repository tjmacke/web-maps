#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] -sel selector -sf shape-file addr-file"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src
WM_SCRIPTS=$WM_HOME/scripts
WM_WORK=$WM_HOME/work

TMP_PFILE=/tmp/pfile.$$
TMP_RNFILE=/tmp/rnums.$$
TMP_LFILE=/tmp/lines.$$
TMP_BB_FILE=/tmp/bboxes.$$
TMP_ANB_FILE=/tmp/a_in_box.$$

USE_BUILD=
SEL=
SHP_FILE=
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
	-sf)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sf requires shape-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SHP_FILE=$1
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
	BOPT="-b"
else
	BINDIR=$WM_BIN
	BOPT=
fi

if [ -z "$SEL" ] ; then
	LOG ERROR "missing -sel selector argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$SHP_FILE" ] ; then
	LOG ERROR "missing -sf shape-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi
# TODO: do this right
SHP_ROOT="$(echo $SHP_FILE | awk -F'.' '{ r = $1 ; for(i = 2; i < NF; i++) { r = r "." $i } ; print r }')"

if [ -z "$FILE" ] ; then
	LOG ERROR "missing addr-file"
	echo "$U_MSG" 1>&2
	exit 1
fi

# 1. decide on which shapes are needed
$SEL $SHP_ROOT.db > $TMP_PFILE
tail -n +2 $TMP_PFILE > $TMP_RNFILE

echo "$TMP_PFILE" 1>&2
head $TMP_PFILE 1>&2
echo 1>&2

# 2. get lines from polys
$BINDIR/shp_to_geojson -sf $SHP_ROOT -pf $TMP_PFILE -pk rnum $TMP_RNFILE	|
$WM_SCRIPTS/get_lines_from_polys.sh						> $TMP_LFILE

# 3. get_bbox.sh
$WM_SCRIPTS/get_bboxes.sh $BOPT -sel $SEL $SHP_FILE	> $TMP_BB_FILE

# 4. add addrs to bbox's.  NB an address may be in > 1 bbox
$BINDIR/find_addrs_in_rect -a $FILE -box $TMP_BB_FILE	|
sort -t $'\t' -k 2,2					|
awk -F'\t' '{
	if($2 != l_2){
		if(l_2 != ""){
			printf("%s", addrs[1])
			for(i = 2; i <= n_addrs; i++){
				n_ary = split(addrs[i], ary, "\t")
				printf("|%s", ary[6])
			}
			printf("\n")
			delete addrs
			n_addrs = 0
		}
	}
	n_addrs++
	addrs[n_addrs] = $0
	l_2 = $2
}
END {
	if(l_2 != ""){
		printf("%s", addrs[1])
		for(i = 2; i <= n_addrs; i++){
			n_ary = split(addrs[i], ary, "\t")
			printf("|%s", ary[6])
		}
		printf("\n")
		delete addrs
		n_addrs = 0
	}
}'> $TMP_ANB_FILE

echo "$TMP_ANB_FILE" 1>&2
head $TMP_ANB_FILE 1>&2
echo 1>&2

# 5. add addrs to nabes selected by bbox's found in 5
if [ -s $TMP_ANB_FILE ] ; then
	echo -e "address\tnabee"
fi
$WM_SCRIPTS/put_point_in_poly.sh -l $TMP_LFILE $TMP_ANB_FILE

rm -f $TMP_PFILE $TMP_RNFILE $TMP_LFILE $TMP_BB_FILE $TMP_ANB_FILE
