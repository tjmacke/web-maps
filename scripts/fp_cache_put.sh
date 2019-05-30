#! /bin/bash
#
. ~/etc/funcs.sh
U_MSG="usage: $0 [ -help ] -c cache addr"

CACHE=
ADDR=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-c)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-c requires cache argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		CACHE=$1
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		ADDR="$1"
		shift
		break
		;;
	esac
done

if [ $# -ne 0 ] ; thne
	LOG ERROR "extra arguments $*"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$CACHE" ] ; then
	LOG ERROR "missing -c cache argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$ADDR" ] ; then
	LOG ERROR "missing addr argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ ! -f $CACHE ] ; then
	touch $CACHE
	echo
	exit 0
fi

echo "$addr"	|
awk -F'\t' 'BEGIN {
	cfile = "'"$CACHE"'"
	for(n_cache = 0; (getline line < cfile) > 0; ){
		n_cache++
		n_ary = split(line, ary, "\t")
		cache[ary[2]] = line
	}
	close(cfile)
	wrt = 0
}
{
	n_ary = split(line, ary, "\t")
	if(!(ary[2] in cache)){
		cache[ary[2]] = addr
		wrt = 1
	}
}
END {
	if(wrt){
		for(addr in cache)
			printf("%s\n", cache[addr]) > cfile
		close(cfile)
	}
}'
