#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -id id-field-name prop-tsv-file"

TMP_BFILE=/tmp/2l.base.$$
TMP_NFILE=/tmp/2l.nabes.$$
TMP_CFILE=/tmp/2l.colors.$$

ID=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
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

if [ -z "$ID" ] ; then
	LOG ERROR "missing -id id-field-name argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$FILE" ] ; then
	LOG ERROR "missing prop-tsv-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

cp $FILE $TMP_BFILE
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
}' $TMP_BFILE	|\
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
	}' $TMP_BFILE	> $TMP_NFILE
	bcolor="$(../scripts/cval_to_cname.sh "$(tail -1 $TMP_NFILE | awk -F'\t' '{ print $NF }')")"
	tail -n +2 $TMP_NFILE	|\
	../bin/shp_to_geojson -sf ../data/Seattle_Neighborhoods/WGS84/Neighborhoods -pf $TMP_NFILE -pk rnum 	|\
	../scripts/find_adjacent_polys.sh -fmt wrapped -id title 						|\
	../scripts/color_graph.sh -bc $bcolor -id title								> $TMP_CFILE 
	../scripts/upd_column_values.sh -mk title -b $TMP_BFILE $TMP_CFILE 
done
tail -n +2 $TMP_BFILE	|\
../bin/shp_to_geojson -sf ../data/Seattle_Neighborhoods/WGS84/Neighborhoods -pf $TMP_BFILE -pk rnum

rm -f $TMP_BFILE $TMP_IDFILE $TMP_NFILE $TMP_CFILE
