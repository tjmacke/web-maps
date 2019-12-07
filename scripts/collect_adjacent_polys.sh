#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -pf pfile [ ap-file ]"

PFILE=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-pf)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-pf requires pfile argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		PFILE=$1
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

if [ -z "$PFILE" ] ; then
	LOG ERROR "missing -pf pfile argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' 'BEGIN {
	l_key[1] = ""
	l_key[2] = ""
	l_key[3] = ""
}
{
	if(trace_f){
		trace_f = 0
		printf("TRACE: BEGIN: select overlapping line segments candidates\n")  > "/dev/stderr"
	}
	key[1] = $3
	key[2] = $4
	key[3] = $5
	if(!keys_equal(key, l_key)){
		if(!key_is_null(l_key)){
			n_ptab = 0
			for(i = 1; i <= n_edges; i++){
				n_ary = split(edges[i], ary, "\t")
				if(!(ary[1] in pset)){
					pset[ary[1]] = 1
					n_ptab++
					ptab[n_ptab] = ary[1]
				}
			}
			if(n_ptab > 1){
				for(i = 1; i < n_ptab; i++){
					for(j = i+1; j <= n_ptab; j++){
						printf("%s\t%s\n", ptab[i], ptab[j])
						printf("%s\t%s\n", ptab[j], ptab[i])
					}
				}
			}
			delete ptab
			delete pset
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
		n_ptab = 0
		for(i = 1; i <= n_edges; i++){
			n_ary = split(edges[i], ary, "\t")
			if(!(ary[1] in pset)){
				pset[ary[1]] = 1
				n_ptab++
				ptab[n_ptab] = ary[1]
			}
		}
		if(n_ptab > 1){
			for(i = 1; i < n_ptab; i++){
				for(j = i+1; j <= n_ptab; j++){
					printf("%s\t%s\n", ptab[i], ptab[j])
					printf("%s\t%s\n", ptab[j], ptab[i])
				}
			}
		}
		delete ptab
		delete pset
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
}' $FILE	|
sort -u		|
awk -F'\t' 'BEGIN {
	pfile = "'"$PFILE"'"
	f_rnum = f_title = -1
	for(n_plines = 0; (getline < pfile) > 0; ){
		n_plines++
		if(n_plines == 1){
			for(i = 1; i <= NF; i++){
				if($i == "rnum")
					f_rnum = i
				else if($i == "title")
					f_title = i
			}
			if(f_rnum == -1){
				printf("ERROR: BEGIN: no field named \"rnum\" in pfile %s\n", pfile) > "/dev/stderr"
				err = 1
				exit err
			}else if(f_title == -1){
				printf("ERROR: BEGIN: no field named \"title\" in pfile %s\n", pfile) > "/dev/stderr"
				err = 1
				exit err
			}
		}else{
			ptab[$f_title] = $f_rnum
			pused[$f_title] = 0
		}
	}
	close(pfile)
	printf("INFO: BEGIN: %s: %d lines\n", pfile, n_plines) > "/dev/stderr"
}
{
	if($1 != l_1){
		if(l_1 != ""){
			pused[l_1] = 1
			printf("%d\t%s\t%d", ptab[l_1], l_1, n_ap)
			if(n_ap > 0){
				printf("\t%s", ap[1])
				for(i = 2; i <= n_ap; i++)
					printf("|%s", ap[i])
			}
			printf("\n")
			n_ap = 0
			delete ap
		}
	}
	l_1 = $1
	n_ap++
	ap[n_ap] = $2
}
END {
	if(err)
		exit err

	if(l_1 != ""){
		pused[l_1] = 1
		printf("%d\t%s\t%d", ptab[l_1], l_1, n_ap)
		if(n_ap > 0){
			printf("\t%s", ap[1])
			for(i = 2; i <= n_ap; i++)
				printf("|%s", ap[i])
		}
		printf("\n")
		n_ap = 0
		delete ap
		for(t in pused){
			if(!pused[t])
				printf("%d\t%s\t0\n", ptab[t], t)
		}
	}
	exit err
}'
