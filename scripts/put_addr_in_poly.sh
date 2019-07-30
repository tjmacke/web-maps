#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -l line-file [ point-file ]"

LFILE=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
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

awk -F'\t' 'BEGIN {
	lfile = "'"$LFILE"'"
	name = ""
	cnt = 0;
	for(n_lines = 0; (getline < lfile) > 0; ){
		n_lines++
		pn_tab[n_lines] = $2 + 0
		iv_tab[n_lines] = $3 + 0
		m_tab[n_lines] = $4 + 0
		b_tab[n_lines] = $5 + 0
		x1 = $6 + 0
		y1 = $7 + 0
		x2 = $8 + 0
		y2 = $9 + 0
		x1_tab[n_lines] = x1
		y1_tab[n_lines] = y1
		x2_tab[n_lines] = x2
		y2_tab[n_lines] = y2
		# create the bounding box
		xmin_tab[n_lines] = min(x1, x2)
		ymin_tab[n_lines] = min(y1, y2)
		xmax_tab[n_lines] = max(x1, x2)
		ymax_tab[n_lines] = max(y1, y2)
		if($1 != name){
			if(name != "")
				l_cnt[name] = cnt
			cnt = 0
			l_first[$1] = n_lines;
		}
		name = $1
		cnt++
	}
	l_cnt[name] = cnt
	close(lfile)
#	for(nb in l_first)
#		printf("%d\t%d\t%s\n", l_first[nb], l_cnt[nb], nb) > "/dev/stderr"
}
{
	n_pcands = split($6, pcands, "|")
	for(pc = 1; pc <= n_pcands; pc++){
		nb = pcands[pc]
		nc = 0
		x = $4 + 0
		y = $5 + 0
		lf = l_first[nb]
		for(l = 0; l < l_cnt[nb]; l++){
			ix = lf + l
			if(y > ymax_tab[ix])
				continue;	# above
			else if(y < ymin_tab[ix])
				continue	# below
			else if(x > xmax_tab[ix])
				continue	# to the right
			nc += crosses(x, y, iv_tab[ix], m_tab[ix], b_tab[ix], x1tab[ix], y1tab[ix], x1tab[ix], y2tab[ix])
		}
		if(nc % 2){
			printf("%s\t%s\n", $2, nb)
			break
		}
	}
}
function min(a, b) {
	return a < b ? a : b
}
function max(a, b) {
	return a > b ? a : b
}
function crosses(x, y, iv, m, b, x1, y1, x2, y2,   ans) {

	if(iv)
		ans =  x < x1
	else if(m != 0)
		ans = x < (y - b)/m
	else
		ans = 0
	return ans
}' $FILE
