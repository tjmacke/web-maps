#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -p url-pfx [ data-list-file ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_DATA=$WM_HOME/data
WM_ZIPS=$WM_DATA/zips

URL_PFX=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-p)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-p requires url-pfx"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		URL_PFX=$1
		shift
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

if [ -z "$URL_PFX" ] ; then
	LOG ERROR "missing -p url-pfx argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [[ ! $URL_PFX =~ /$ ]] ; then
	URL_PFX="$URL_PFX/"
fi

cat $FILE	|\
while read line ; do
	if [[ $line =~ ^# ]] ; then
		LOG INFO "skip file $line"
		continue
	fi
	LOG INFO "get file ${URL_PFX}$line"
	curl -s -S "${URL_PFX}$line" > $WM_ZIPS/$line
	LOG INFO "got file ${URL_PFX}$line"
done
