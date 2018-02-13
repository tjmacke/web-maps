#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ dbf-pat ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data
WM_INDEXED=$WM_DATA/indexed

DBF_PAT=

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
		DBF_PAT="$1"
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

if [ -z "$DBF_PAT" ] ; then
	FLIST="$(ls $WM_INDEXED/*.dbf)"
else
	FLIST="$(ls $WM_INDEXED/*.dbf |\
		awk -F/ 'BEGIN {
			dbf_pat = "'"$DBF_PAT"'"
		}
		{
			if($NF ~ dbf_pat)
				print $0
		}')"
fi

if [ -z "$FLIST" ] ; then
	LOG ERROR "dbf_pat \"$DBF_PAT\" did not select any files"
	exit 1
fi

for dbf in $FLIST ; do
	db="$(echo "$dbf" | awk '{ printf("%s\n", substr($0, 1, length($0) - 1)) }')"
	LOG INFO "convert $dbf to $db"
	$WM_BIN/dbase_to_sqlite -t data -pk +rnum -ktl statefp.tsv $dbf 2> /dev/null | sqlite3 $db
	LOG INFO "converted $dbf to $db"
done
