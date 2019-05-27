#! /bin/bash
#
. ~/etc/funcs.sh
U_MSG="usage: $0 [ -help ] -db db-file addr"

FP_DB=
ADDR=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-db)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-db requires db-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FP_DB="$1"
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

if [ $# -ne 0 ] ; then
	LOG ERROR "extra arguments $*"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$FP_DB" ] ; then
	LOG ERROR "missing -db db-file argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ ! -s $FP_DB ] ; then
	LOG ERROR "database $FP_DB either does not exist or has zero size"
	exit 1
fi

# make the SELECT stmt
sel_stmt="$(echo "$ADDR" |
	awk 'BEGIN {
		apos = sprintf("%c", 39)
	}
	{
		addr = $0
		gsub(apos, apos apos, addr)
		printf(".mode tab\\n")
		printf("SELECT date(%snow%s), address, %s.%s, lng, lat, rply_address\\n", apos, apos, apos, apos)
		printf("FROM addresses\\n")
		printf("WHERE address = %s%s%s AND as_reason LIKE %sgeo.ok.%%%s ;", apos, addr, apos, apos, apos)
	}')"
# query the DB
rply="$(echo -e "$sel_stmt" | sqlite3 $FP_DB)"
echo "$rply"
exit 0
