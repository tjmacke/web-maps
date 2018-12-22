#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] data-file-1 ..."

FILES=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		if [ -z "$FILES" ] ; then
			FILES=$1
		else
			FILES="$FILES $1"
		fi
		shift
		;;
	esac
done

if [ -z "$FILES" ] ; then
	LOG ERROR "no data-files"
	echo "$U_MSG" 1>&2
	exit 1
fi

for f in $FILES; do
	awk '{
		if(fd == ""){
			nf = split(FILENAME, ary, ".")
			fd = ary[nf-1]
		}
		work = $0
		sub(/^ */, "", work)
		sub(/ *$/, "", work)
		sp = index(work, " ")
		cmd_num = substr(work, 1, sp - 1) + 0
		work = substr(work, sp + 1)
		sub(/^ */, "", work)
		printf("%d\t%s\t%s\n", cmd_num, fd, work)
	}' $f
done	|
sort -t $'\t' -k 1n,1 -k 2n,2	|
awk -F'\t' '{
	if($1 != l_1){
		if(l_1 != ""){
			if(n_dups > 1)
				analyze_dups(n_dups, dups)
			delete dups
			n_dups = 0
		}
	}
	if($3 ~ /\/find_parking.sh/){
		if($3 ~ / -a /){
			n_dups++
			dups[n_dups] = $0
		}
	}
	l_1 = $1
}
END {
	if(l_1 != ""){
		if(n_dups > 1)
			analyze_dups(n_dups, dups)
		delete dups
		n_dups = 0
	}
}
function analyze_dups(n_dups, dups,   i, n_ary, ary, d_idx, nd_tab, d_tab) {

	for(i = 1; i <= n_dups; i++){
		n_ary = split(dups[i], ary, "\t")
		key = sprintf("%s\t%s", ary[1], ary[3])
		if(!(key in d_idx)){
			nd_tab++
			d_idx[key] = nd_tab
			d_tab[nd_tab] = dups[i] 
		}
	}

	if(nd_tab > 1){
		for(i = 1; i <= nd_tab; i++){
			n_ary = split(d_tab[i], ary, "\t")
			printf("%s.%d\t%s\t%s\n", ary[1], i, ary[2], ary[3])
		}
	}else
		printf("%s\n", d_tab[1])
}'
