#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -geo geocoder ] { -a address | [ address-file ] }"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data/Seattle_Parking

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME not defined"
	exit 1
fi
DM_SCRIPTS=$DM_HOME/scripts

# very simple config, unlikely to ever change, so just create it in line
TMP_FP_CFILE=/tmp/fp_cfile.$$
cat > $TMP_FP_CFILE <<_EOF_
main.scale_type = factor
main.values = 0.90,0.90,0.90 | 0.94,0.94,0.5 | 0.94,0.75,0.5 | 0.94,0.5,0.5
main.keys = PPL | PLU | PCVL | PTRKL
main.def_value = 0.63,0.63,0.94
main.def_key_text = dest
_EOF_

TMP_OFILE=/tmp/out.$$
TMP_EFILE=/tmp/err.$$
TMP_PFILE=/tmp/pspots.$$
TMP_CFILE=/tmp/cfile.$$
TMP_SC_FILE=/tmp/sc_file.$$

GEO=
ADDR=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-geo)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-geo requires geocoder argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		GEO=$1
		shift
		;;
	-a)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-a requires address argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		ADDR="$1"
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

if [ ! -z "$GEO" ] ; then
	GEO="-geo $GEO"
fi

if [ ! -z "$ADDR" ] ; then
	if [ ! -z "$FILE" ] ; then
		LOG ERROR "-a address not allowed with address-file"
		echo "$U_MSG" 1>&2
		exit 1
	fi
	AOPT="-a"
else
	AOPT=""
fi

$DM_SCRIPTS/get_geo_for_addrs.sh $GEO $AOPT "$ADDR" > $TMP_OFILE 2> $TMP_EFILE

n_OFILE=$(cat $TMP_OFILE | wc -l)
n_EFILE=$(grep ERROR: $TMP_EFILE | wc -l)
n_ADDRS=$((n_OFILE + n_EFILE))
if [ $n_EFILE -ne 0 ] ; then
	LOG ERROR "$n_OFILE/$n_ADDRS found"
	cat $TMP_EFILE 1>&2
fi
if [ $n_OFILE -ne 0 ] ; then
	$WM_BIN/find_addrs_in_rect -a $WM_DATA/sps_sorted.tsv $TMP_OFILE > $TMP_PFILE
	awk -F'\t' '
	@include "/Users/tom/work/dd_maps/lib/cfg_utils.awk"
	@include "/Users/tom/work/dd_maps/lib/interp_utils.awk"
	@include "/Users/tom/work/dd_maps/lib/color_utils.awk"
	BEGIN {
		cfile = "'"$TMP_FP_CFILE"'"
		CFG_read(cfile, config)
		
		if(IU_init(config, color, "main")){
			printf("ERROR: BEGIN: IU_init failed\n") > "/dev/stderr"
			err = 1
			exit err
		}
	}
	{
		iv = IU_interpolate(color, $NF)
		printf("#%s\n", CU_rgb_to_24bit_color(iv))
	}' $TMP_PFILE > $TMP_CFILE
	$DM_SCRIPTS/cfg_to_json.sh $TMP_FP_CFILE > $TMP_SC_FILE
	$DM_SCRIPTS/map_addrs.sh -sc $TMP_SC_FILE -cf $TMP_CFILE -at src $TMP_PFILE
fi

rm -f $TMP_FP_CFILE $TMP_OFILE $TMP_EFILE $TMP_PFILE $TMP_CFILE $TMP_SC_FILE
