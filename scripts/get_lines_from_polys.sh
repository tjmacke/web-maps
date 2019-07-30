#! /bin/bash
#
. ~/etc/funcs.sh
U_MSG="usage: $0 [ -help ] [ -trace ] [ geosjson-file ]"

JU_HOME=$HOME/json_utils
JU_BIN=$JU_HOME/bin

JG='{geojson}{features}[1:$]'
JG2='{properties}{title},{geometry}{type,coordinates}'

TRACE=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-trace)
		TRACE="yes"
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

$JU_BIN/json_get -g $JG $FILE 2> /dev/null	|
$JU_BIN/json_get -n -g $JG2 2> /dev/null	|
awk -F'\t' 'BEGIN {
	# TODO: figure out what to do with trace
	trace = "'"$TRACE"'" == "yes"
	if(trace){
		trace_f = 1
		at_file = "/dev/stderr"
	}
}
{
	if(trace_f){
		trace_f = 0
		printf("TRACE: BEGIN: process json into line segments\n") > "/dev/stderr"
	}
	all_titles[$1] = 1
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
END {
	if(trace){
		printf("TRACE: END: processed json into line segments\n") > "/dev/stderr"
		for(t in all_titles)
			printf("%s\n", t) > at_file
		close(at_file)
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
}'
