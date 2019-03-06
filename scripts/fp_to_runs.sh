#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ fp-mon-dir ]"

DIR=

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
		DIR=$1
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

if [ -z "$DIR" ] ; then
	LOG ERROR "missing fp-mon-dir argumnent"
	echo "$U_MSG" 1>&2
	exit 1
fi

cat $DIR/*.log	|
awk '{
	n_lines++
	lines[n_lines] = $0
}
END {
	for(i = 1; i <= n_lines; i++){
		mk_job(lines, i)
	}
}
function mk_job(lines, ln,   ainfo, da_start, d) {

	parse_aline(lines[ln], ainfo)
	da_start = set_da_start(lines, ln, ainfo)
	for(d = da_start; d <= ainfo["n_atab"]; d++){
		printf("%s\t%s", ainfo["date"], ainfo["time"])
		printf("\t.\t.\tJob")
		printf("\t%s\t%s", ainfo["atab", 1], ainfo["atab", d])
		printf("\t.\t%s\t.", ainfo["app"])
		printf("\n")
	}
}
function parse_aline(line, ainfo,   n_ary, ary, n_dt, dt, i) {
	n_ary = split(line, ary)
	n_dt = split(ary[1], dt, "_")
	ainfo["date"] = sprintf("%s-%s-%s", substr(dt[1], 1, 4), substr(dt[1], 5, 2), substr(dt[1], 7, 2))
	ainfo["time"] = sprintf("%s:%s", substr(dt[2], 1, 2), substr(dt[2], 3, 2))
	ainfo["app"] = "gh"
	ainfo["alist"] = ""
	for(i = 2; i <= n_ary; i++){
		if(ary[i] == "-app"){
			i++
			ainfo["app"] = ary[i]
		}else if(ary[i] == "-a"){
			i++
			ainfo["alist"] = ary[i]
		}else if(ainfo["alist"]){
			ainfo["alist"] = ainfo["alist"] " " ary[i]
		}
	}
	n_ary = split(ainfo["alist"], ary, "|")
	ainfo["n_atab"] = n_ary
	for(i = 1; i <= n_ary; i++){
		if(i == 1)
			sub(/^rest:  */, "", ary[i])
		else
			sub(/^  *[1-9]\.  */, "", ary[i])
		sub(/  *$/, "", ary[i])
		ainfo["atab", i] = ary[i]
	}
}
function set_da_start(lines, ln, ainfo,   p_ainfo) {
	if(ln == 1)
		return 2
	else if(ainfo["n_atab"] == 2)
		return 2
	else{
		parse_aline(lines[ln-1], p_ainfo)

#		printf("pfx = %s\n", p_ainfo["alist"])
#		printf("me  = %s\n", ainfo["alist"])

		if(index(ainfo["alist"], p_ainfo["alist"]) == 1)
			return p_ainfo["n_atab"] + 1
		else
			return 2
	}
}'
