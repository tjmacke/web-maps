#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -is_chull ] -l line-file [ point-file ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi

WM_LIB=$WM_HOME/lib

IS_CHULL=0
LFILE=
FILE=

# line file format: tsv, can contain >= 1 polygon
# Fnum	Fval
# 1	polygon name
# 2	polygon number
# 3	is vertical, so slope, m is inf and y intercept, b is the x intercept
# 4	slope, m. see #3	
# 5	y intercept, b, see #3
# 6	x1
# 7	y1
# 8	x2
# 9	y2

# points file formt: tsv
# Fnum	Fval
# 1	User defined
# 2	point ID
# 3	User defined
# 4	x
# 5	y
# 6	candidate polys: as a "|" separated list

# output, which may be enpty if no pts are in any of the polys
# output file format: tsv
# Fnum	Fval
# 1	point ID
# 2	Name of polygon containing this point

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-is_chull)
		IS_CHULL=1
		shift
		;;
	-l)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-l requires line-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		LFILE=$1
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

if [ -z "$LFILE" ] ; then
	LOG ERROR "missing -l line-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' -i $WM_LIB/wm_utils.awk -v is_chull="$IS_CHULL" -v lfile="$LFILE" 'BEGIN {
        emsg = WM_read_polys(lfile, is_chull, iv_tab, m_tab, b_tab, x1_tab, xmin_tab, ymin_tab, xmax_tab, ymax_tab, l_first, l_cnt, chull)
	if(emsg != "")
		exit 1
}
{
	x = $4 + 0
	y = $5 + 0
	n_pcands = split($6, pcands, "|")
	WM_put_point_in_poly(is_chull, n_pcands, pcands, x, y, iv_tab, m_tab, b_tab, x1_tab, xmin_tab, ymin_tab, xmax_tab, ymax_tab, l_first, l_cnt, chull,    pc, nb, nc, lf, l, ix)
}
END {
	if(err)
		exit emsg
	exit 0
}' $FILE
