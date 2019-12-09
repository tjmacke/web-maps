#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] DB"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
# TODO: generalize this 
WM_DATA=$WM_HOME/data/cb_2016_place_500k
WM_ETC=$WM_HOME/etc

STATE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-*)
		LOG_ERROR "unknown option $1"
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
	exit 1
fi

sqlite3 $DB <<_EOF_
.headers on
.mode tabs
select rnum, NAME || ', ' || statefp.STUSAB || '_' || PLACEFP as title from data, statefp where data.STATEFP = statefp.STATEFP ;
_EOF_
