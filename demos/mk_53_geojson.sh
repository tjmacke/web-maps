#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] [ -sa adj-file ] (no arguments)"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src
WM_DATA=$WM_HOME/data
WM_DEMOS=$WM_HOME/demos
WM_SCRIPTS=$WM_HOME/scripts
WA_DATA=$WM_DATA/cb_2016_place_500k/cb_2016_53_place_500k.shp

BOPT=
SA=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-b)
		BOPT="-b"
		shift
		;;
	-sa)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sa requires adj-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SA="-sa $1"
		shift
		;;
	-*)
		LOG_ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		LOG_ERROR "extra arguments $*"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	esac
done

$WM_SCRIPTS/map_shapes.sh $BOPT $SA -sel $WM_DEMOS/select_places.sh $WA_DATA
