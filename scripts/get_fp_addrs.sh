#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ file ] "

FILE=

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

awk 'BEGIN {
	apos = sprintf("%c", 39)
}
{
	work = $0
	idx = index(work, " -a ")
	work = substr(work, idx+4)
	idx = index(work, ">")
	if(idx != 0)
		work = substr(work, 1, idx-1)
	sub(/^  */, "", work)
	sub(/  *$/, "", work)
	o_quote = substr(work, 1, 1)
	c_quote = substr(work, length(work))
	emsg = bad_quotes(o_quote, c_quote)
	if(emsg != "")
		printf("WARN: main: line %7d: %s: %s\n", NR, emsg, work) > "/dev/stderr"
	if(o_quote == apos || o_quote == "\"")
		work = substr(work, 2)
	if(c_quote == apos || c_quote == "\"")
		work = substr(work, 1, length(work) - 1)
	sub(/^  */, "", work)
	sub(/  *$/, "", work)

	n_ary = split(work, ary, "|")
	for(i = 1; i <= n_ary; i++){
		sub(/^  */, "", ary[i])
		if(match(ary[i], /^[1-9]\./)){
			ary[i] = substr(ary[i], RLENGTH+1)
			sub(/^  */, "", ary[i])
		}
		sub(/  *$/, "", ary[i])
		ary[i] = clean_addr(ary[i])
		printf("%s\n", ary[i])
	}
}
function bad_quotes(o_quote, c_quote) {
	if(o_quote == apos){
		if(o_quote == c_quote)
			return ""
		else
			return sprintf("missing closing %s", apos)
	}
	if(o_quote == "\""){
		if(o_quote == c_quote)
			return ""
		else
			return sprintf("missing closing %s", "\"")
	}
	if(c_quote == apos)
		return sprintf("missing opening %s", apos)
	if(c_quote == "\"")
		return sprintf("missing opening %s", "\"")
	return "unquoted address"
} 
function clean_addr(addr,   i, n_ary, ary, new_addr, j, n_ary2, ary2, new_apart) {
	n_ary = split(addr, ary, ",")
	for(i = 1; i <= n_ary; i++){
		sub(/^  */, "", ary[i])
		sub(/  *$/, "", ary[i])
	}
	if(ary[n_ary-1] == "Seattle" && ary[n_ary] == "WA")
		n_ary--;
	new_addr = ""
	for(i = 1; i <= n_ary; i++){
		n_ary2 = split(ary[i], ary2, /  */)
		new_apart = ""
		for(j = 1; j <= n_ary2; j++){
			sub(/^  */, "", ary2[j])
			sub(/  *$/, "", ary2[j])
			new_apart = new_apart (j > 1 ? " " : "") ary2[j]
		}
		new_addr = new_addr (i > 1 ? ", " : "") new_apart
	}
	return new_addr
}' $FILE
