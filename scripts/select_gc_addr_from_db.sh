#! /bin/bash
#
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


#
# ADDR is a pipe (|) separated list of addresses, each of which may have an
# optional prefix that controls how it will be displayed on the map
# Examples:
# 
# 1.	addr
# 2.	addr | addr ...
#
# 1 is the simplest form, a single unprefixed address.
# 2 is a list of unprefixed addresses.  
#
# 3.	rest: addr | 1. addr
#
# 3 uses 2 prefixes:  rest: to indiate this addr is a restaurant and and will
# be colored green and 1. indicating the 2d address is diner; this address will
# be colored purple
#
# 4.	rest: addr | 1. addr | 2. addr ...
#
# A list of a prefixed addressed.  The numbers indicate the order in whch the 
# pickups were received and usually but not always the order in which they are
# to be delivered
#
# 5.	rest: addr | 1. addr | 2. addr ... | last addr
#
# A list of prefixed address, where last indicates that last deliver addresss
# of the previous delivery which will be colored gray
#
echo "$ADDR"	|
awk -F'|' '{
	# split on pipe into an array of possibly prefixed addresses
	# trim leading/trailing spaces
	# find addr prefixes (if any)
	for(i = 1; i <= NF; i++){
		sub(/^  */, "", $i)
		sub(/  *$/, "", $i)
		if(match($i, /^rest:  */)){
			pfx = substr($i, 1, RLENGTH)
			addr = substr($i, RLENGTH + 1)
		}else if(match($i, /^[1-9]\.  */)){
			pfx = substr($i, 1, RLENGTH)
			addr = substr($i, RLENGTH + 1)
		}else{	# no pfx
			pfx = ""
			addr = $i
		}
		printf("%s\t%s\n", pfx, addr)
	}
}'	|
while read line ; do
	pfx="$(echo "$line" | awk -F'\t' '{ print $1 }')"
	addr="$(echo "$line" | awk -F'\t' '{ print $2 }')"
	# make the SELECT stmt
	qry="$(echo "$addr" |
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
	rply="$(echo -e "$qry" | sqlite3 $FP_DB)"
	if [ -z "$rply" ] ; then
		echo ''
	else
		# reinsert the address prefix
		echo "$pfx|$rply" | awk -F'|' '{
			if($1 == ""){
				print $2
			}else{
				n_ary = split($2, ary, "\t")
				ary[2] = $1 ary[2]
				printf("%s", ary[1])
				for(i = 2; i <= n_ary; i++)
					printf("\t%s", ary[i])
				printf("\n")
			}
		}'
	fi
done
