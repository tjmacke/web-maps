#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] (no arguments)"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_DATA=$WM_HOME/data
WM_DEMOS=$WM_HOME/demos
WM_SCRIPTS=$WM_HOME/scripts

SHP_ROOT=$WM_DATA/Paris/arrondissements

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
		LOG ERROR "extra arguments $*"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	esac
done

$WM_SCRIPTS/map_shapes.sh -sel $WM_DEMOS/select_paris.sh $SHP_ROOT

exit 1
