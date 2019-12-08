#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -fmt { wrapped | bare } -id id-field [ geojson-file ]"

# require JU_HOME to be defined?
JU_HOME=$HOME/json_utils
JU_BIN=$JU_HOME/bin

TMP_JG2_FILE=/tmp/jg_2.$$

FMT=
ID=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-fmt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-fmt requires format-value"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FMT=$1
		shift
		;;
	-id)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-id requires id-field argument"
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

if [ -z "$FMT" ] ; then
	LOG ERROR "missing -fmt format-value argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ "$FMT" == "wrapped" ] ; then
	JG='{geojson}{features}[1:$]'
elif [ "$FMT" == "bare" ] ; then
	JG='{features}[1:$]'
else
	LOG ERROR "unknown format-value $FMT, must wrapped or bare"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$ID" ] ; then
	LOG ERROR "missing -id id-field argument"
	echo "$U_MSG" 1>&2
	exit 1
else
	# Can't figure out how to escape this, so put in a file
	echo "$ID" | awk '{ printf("{properties}{%s}, {geometry}{type, coordinates}", $1) }' > $TMP_JG2_FILE
fi

$JU_BIN/json_get -g $JG $FILE 2> /dev/null		|\
$JU_BIN/json_get -n -g @$TMP_JG2_FILE 2> /dev/null	|\
awk -F'\t' '{
	nf = split($3, ary, "]")
	n_polys = n_points = 0
	for(i = 1; i < nf; i++){
		if(index(ary[i], "[[")){
			n_polys++
			n_points++
			titles[n_polys] = $1
			p_first[n_polys] = n_points;
			p_count[n_polys] = 1
			get_xy(ary[i], n_points, x, y)
		}else if(index(ary[i], "[")){
			n_points++
			p_count[n_polys]++
			get_xy(ary[i], n_points, x, y)
		}
	}
	for(p = 1; p <= n_polys; p++){
		for(i = 0; i < p_count[p] - 1; i++){
			x1 = x[p_first[p]+i]
			x2 = x[p_first[p]+i+1]
			y1 = y[p_first[p]+i]
			y2 = y[p_first[p]+i+1]
			dx = x2 - x1
			dy = y2 - y1
			printf("%s\t%d\t%d", $1, p, dx == 0)
			if(dx == 0){
				m = 0
				b = x1
			}else{
				m = dy/dx
				b = y1 - m*x1
			}
			printf("\t%.15e\t%.15e\t%.15e\t%.15e\t%.15e\t%.15e", m, b, x1, y1, x2, y2)
			printf("\n")
		}
	}
}
function get_xy(str, n_points, x, y,   work, idx, x_str, y_str) {
	work = str
	idx = match(work, /[0-9-]/)
	work = substr(work, idx)
	idx = index(work, ",")
	x_str = substr(work, 1, idx-1)
	y_str = substr(work, idx+1)
	x[n_points] = x_str + 0
	y[n_points] = y_str + 0
}
function abs(x) {
	return x < 0 ? -x : x
}'					|
sort -t $'\t' -k 3,3 -k 4g,5 -k 5g,5	|	# add a tee here to collect the sorted edge data
awk -F'\t' 'BEGIN {
	l_key[1] = ""
	l_key[2] = ""
	l_key[3] = ""
}
{
	key[1] = $3
	key[2] = $4
	key[3] = $5
	if(!keys_equal(key, l_key)){
		if(!key_is_null(l_key)){
			if(n_edges > 1){
				for(i = 1; i <= n_edges; i++)
					printf("%s\n", edges[i])
				printf("\n")
			}
			delete edges
			n_edges = 0
		}
	}
	n_edges++
	edges[n_edges] = $0
	key_assign(l_key, key)
}
END {
	if(!key_is_null(l_key)){
		if(n_edges > 1){
			for(i = 1; i <= n_edges; i++)
				printf("%s\n", edges[i])
		}
		delete edges
		n_edges = 0
	}
}
function keys_equal(k1, k2) {

	return (k1[1] == k2[1] && k1[2] == k2[2] && k1[3] == k2[3])
}
function key_is_null(k) {
	return (k[1] == "" && k[2] == "" && k[3] == "")
}
function key_assign(kd, ks) {
	kd[1] = ks[1]
	kd[2] = ks[2]
	kd[3] = ks[3]
}'

rm -f $TMP_JG2_FILE
