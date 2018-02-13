#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -db dbase { -q query | query-file }"

DBASE=
QUERY=
QFILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-db)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-db requires dbase argument"
			echo "$U_USG" 1>&2
			exit 1
		fi
		DBASE=$1
		shift
		;;
	-q)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-q requires query argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		QUERY="$1"
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		QFILE=$1
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

# TODO: allow for pfx as shape files have lots of related files
if [ -z "$DBASE" ] ; then
	LOG ERROR "missing -db dbase argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ ! -z "$QUERY" ] ; then
	if [ ! -z "$QFILE" ] ; then
		LOG ERROR "-q query not allowed with query-file"
		echo "$U_MSG" 1>&2
		exit 1
	fi
	echo "$QUERY"
else
	cat "$QFILE"
fi	|\
sqlite3 -separator $'\t' -cmd '.headers on' $DBASE
