#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -h [ -d { lines*|colors|union } ] ] (no arguments)"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_DATA=$WM_HOME/data
WM_DEMOS=$WM_HOME/demos
WM_SCRIPTS=$WM_HOME/scripts

HOPT=
HDISP=

SHP_ROOT=$WM_DATA/London-wards-2014/london

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
	-d)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-d requires display argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		HDISP=$1
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		LOG ERROR "extra arguments $*"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	esac
done

if [ "$HOPT" == "yes" ] ; then
	HOPT="-h"
	if [ -z "$HDISP" ] ; then
		HDISP="-d lines"
	elif [ "$HDISP" == "union" ] ; then
		LOG ERROR "-d union not yet implemented, use lines or colors"
		exit 1
	elif [ "$HDISP" == "colors" ] ; then
		HDISP="-d colors"
	elif [ "$HDISP" != "lines" ] ; then
		LOG ERROR "bad hierarchy display type $HDISP, must be one of union, lines or colors"
		echo "$U_MSG" 1>&2
		exit 1
	fi
elif [ ! -z "$HDISP" ] ; then
	LOG ERROR "-d requires -h option"
	echo "$U_MSG" 1>&2
	exit 1
fi

$WM_SCRIPTS/map_shapes.sh $HOPT $HDISP -sel $WM_DEMOS/select_lw.sh $SHP_ROOT

exit 0
