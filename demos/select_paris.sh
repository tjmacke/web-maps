#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] db-file"

DB=

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
		DB=$1
		shift
		break
		;;
	esac
done

if [ $# -ne 0 ] ; then
	LOG ERROR "extra arguments $*"
	echo "$U_MSG" 1>&2
	exit 1;
fi

if [ -z "$DB" ] ; then
	LOG ERROR "missing db-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

sqlite3 $DB <<_EOF_ |
.headers on
.mode tab
select rnum, l_ar as ar_num, l_aroff as ar_name from data ;
_EOF_
awk -F'\t' 'NR == 1 {
	for(i = 1; i <= NF; i++)
		fnums[$i] = i
}
NR > 1 {
	n_recs++
	recs[n_recs] = $0
}
END {
	printf("%s\t%s\n", "rnum", "title")
	for(i = 1; i <= n_recs; i++)
		printf("%d\t%s\n", i, mk_title(fnums, recs[i]))
}
function mk_title(fnums, rec,   n_ary, ary, n_ary2, ary2) {
	n_ary = split(rec, ary)
	n_ary2 = split(ary[fnums["ar_num"]], ary2, /  */)
	return sprintf("%-6s %s", (ary2[1] ":"), ary[fnums["ar_name"]])
}'
