#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -h ] db-file"

HOPT=
DB=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-h)
		HOPT="yes"
		shift
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

if [ $# -ne 0  ] ; then
	LOG ERROR "extra arguments $*"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$DB" ] ; then
	LOG ERROR "mmissing db-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

sqlite3 $DB <<_EOF_	|
.headers on
.mode tabs
select OBJECTID, L_HOOD, S_HOOD from data order by L_HOOD, S_HOOD ;
_EOF_
awk -F'\t' 'BEGIN {
	hopt = "'"$HOPT"'" == "yes"
}
NR == 1 {
	for(i = 1; i <= NF; i++)
		fnums[$i] = i
}
NR > 1 {
	n_recs++
	recs[n_recs] = $0
}
END {
	for(i = 1; i <= n_recs; i++){
		titles[i] = title = mk_title(fnums, recs[i])
		if(title == "")
			continue;
 		t_count[title]++
		if(hopt){
			ids[i] = id = mk_id(fnums, recs[i])
			i_count[id]++
		}
	}
	printf("%s\t%s", "rnum", "title")
	if(hopt)
		printf("\t%s", "id")
	printf("\n")
	for(i = 1; i <= n_recs; i++){
		title = titles[i]
		if(title == "")
			continue
		nf = split(recs[i], ary)
		if(t_count[title] > 1){
			if(title !~ /\/$/)
				title = title "_" ary[fnums["OBJECTID"]]
		}
		printf("%s\t%s", ary[fnums["OBJECTID"]], title)
		if(hopt){
			id = mk_id(fnums, recs[i])
			if(i_count[id] > 1){
				if(id !~ /\/$/)
					id = id "_" ary[fnums["OBJECTID"]]
			}
			printf("\t%s", id)
		}
		printf("\n")
	}
}
function mk_title(fnums, rec,   nf, ary, i, title) {

	nf = split(rec, ary)
	if(ary[fnums["L_HOOD"]] == "")
		title = ""
	else if(ary[fnums["L_HOOD"]] == "NO BROADER TERM")
		title = ary[fnums["S_HOOD"]]
	else
		title = sprintf("%s/%s", ary[fnums["L_HOOD"]], ary[fnums["S_HOOD"]])
	return title
}
function mk_id(fnums, rec,   nf, ary, i, id) {

	nf = split(rec, ary)
	if(ary[fnums["L_HOOD"]] == "")
		id = ""
	else if(ary[fnums["L_HOOD"]] == "NO BROADER TERM")
		id = ary[fnums["S_HOOD"]] 
	else
		id = sprintf("%s/", ary[fnums["L_HOOD"]])
	return id
}'
