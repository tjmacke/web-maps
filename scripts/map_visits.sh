#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -c cfg-file -at { src | dst } [ -w where ] [ -u { weeks* | days } ] db"

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME not defined"
	exit 1
fi

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi

DM_SCRIPTS=$DM_HOME/scripts
WM_SCRIPTS=$WM_HOME/scripts
WM_DATA=$WM_HOME/data/Seattle_Parking
SN_FLIST=$WM_DATA/sn.list.json

TMP_MFILE=/tmp/mf.$$
TMP_JC_FILE=/tmp/mf.$$.json
TMP_MBD=/tmp/mbd.$$.tsv
TMP_PFILE=/tmp/pfile.$$.tsv

CFILE=
ATYPE=
WHERE=
UNIT=
MFILE=
DB=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-c)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-c requires cfg-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		CFILE=$1
		shift
		;;
	-at)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-at requires address-type argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		ATYPE=$1
		shift
		;;
	-w)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-w requires where clause argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		WHERE="$1"
		shift
		;;
	-u)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-u requires unit argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		UNIT=$1
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
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

if [ -z "$CFILE" ] ; then
	LOG ERROR "missing -c cfg-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$ATYPE" ] ; then
	LOG ERROR "missing -at address-type argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ "$ATYPE" != "src" ] && [ "$ATYPE" != "dst" ] ; then
	LOG ERROR "unknown address-type $ATYPE, must src or dst"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ ! -z "$WHERE" ] ; then
	WHERE="-w $WHERE"
fi

if [ ! -z "$UNIT" ] ; then
	if [ "$UNIT" == "days" ] ; then
		UNIT="-u $UNIT"
		BK="days"
	elif [ "$UNIT" == "weeks" ] ; then
		BK="weeks"
	else
		LOG ERROR "unknown unit \"$UNIT\", must be weeks or days" 
		echo "$U_MSG" 1>&2
		exit 1
	fi
else
	BK="weeks"
fi

$DM_SCRIPTS/get_visit_info.sh -at $ATYPE $WHERE $UNIT -meta $TMP_MFILE $DB				|
$WM_SCRIPTS/interp_prop.sh -c $CFILE -bk $BK -nk marker-color -meta $TMP_MFILE	| $WM_SCRIPTS/format_color.sh -k marker-color	|
$WM_SCRIPTS/interp_prop.sh -c $CFILE -bk visits -nk marker-size -meta $TMP_MFILE	> $TMP_MBD
cat $CFILE $TMP_MFILE | $DM_SCRIPTS/cfg_to_json.sh > $TMP_JC_FILE

# extract relevant properties (address, title, marker-color, marker-size)
$WM_SCRIPTS/extract_cols.sh -c "address,title,marker-color,marker-size" $TMP_MBD > $TMP_PFILE
# extract relevant properties to an address-geo format (text, address, text, lng, lat, text)
$WM_SCRIPTS/extract_cols.sh -hdr drop -c "last,address,visits,lng,lat,$BK" $TMP_MBD	|
$WM_SCRIPTS/map_addrs.sh -sc $TMP_JC_FILE -p $TMP_PFILE -mk address -fl $SN_FLIST -at src

rm -f $TMP_MFILE $TMP_JC_FILE $TMP_MBD $TMP_PFILE
