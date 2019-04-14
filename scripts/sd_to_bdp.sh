#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ sd-file ]"

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

grep -v date $FILE	|
sort -t $'\t' -k 1,1 -k 3,3	|
awk -F'\t' 'BEGIN {
	pr_hdr = 1
	hdr = "date\ttime_start\ttime_end\tdeliveries\thours\tdelivery_pay\tboost_pay\ttip_amount\tdeductions\textras\ttotal_pay\thrate\tdrate\tdph"
}
{
	if(pr_hdr){
		pr_hdr = 0
		printf("%s\n", hdr)
	}
	printf("%s", $1)
	printf("\t%s", $3)
	printf("\t%s", $4)
	printf("\t%s", $7)
	hrs = (hm2min($4) - hm2min($3))/60.0
	printf("\t%.2f", hrs)
	# next 5 fields were DD exclusive, so just used -1 to indicate N/A
	for(i = 1; i <= 5; i++)
		printf("\t-1")
	printf("\t%s", $8 == "." ? "-1" : $8)
	if($8 != ".")
		printf("\t%.2f\t%.2f", $8/hrs, $8/$7)
	else	# use -1 to indiate N/A
		printf("\t-1\t-1")
	printf("\t%.2f", $7/hrs)
	printf("\n")
}
function hm2min(hm,   n_ary, ary) {
	h_ary = split(hm, ary, ":")
	return 60*ary[1] + ary[2]
}'
