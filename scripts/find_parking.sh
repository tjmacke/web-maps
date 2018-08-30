#  /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -v ] [ -d D ] { -a address | [ address-file ] }"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data/Seattle_Parking
WM_SRC=$WM_HOME/src

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME not defined"
	exit 1
fi
DM_LIB=$DM_HOME/lib
DM_SCRIPTS=$DM_HOME/scripts

# awk v3 does not support include
AWK_VERSION="$(awk --version | awk '{ nf = split($3, ary, /[,.]/) ; print ary[1] ; exit 0 }')"
if [ "$AWK_VERSION" == "3" ] ; then
	AWK="igawk --re-interval"
	CFG_UTILS="$DM_LIB/cfg_utils.awk"
	INTERP_UTILS="$DM_LIB/interp_utils.awk"
	COLOR_UTILS="$DM_LIB/color_utils.awk"
elif [ "$AWK_VERSION" == "4" ] ; then
	AWK=awk
	CFG_UTILS="\"$DM_LIB/cfg_utils.awk\""
	INTERP_UTILS="\"$DM_LIB/interp_utils.awk\""
	COLOR_UTILS="\"$DM_LIB/color_utils.awk\""
else
	LOG ERROR "unsupported awk version: \"$AWK_VERSION\": must be 3 or 4"
	exit 1
fi


TMP_AFILE=/tmp/addrs.$$		# addrs for 1st geocoder
TMP_AFILE_2=/tmp/addrs_2.$$	# addrs for 2nd geocoder
TMP_OFILE=/tmp/out.$$		# output of 1st geocoder
TMP_OFILE_2=/tmp/out_2.$$	# output of 2nd geocoder, eventually appended to TMP_OFILE
TMP_EFILE=/tmp/err.$$		# errs for 1st, 2nd geocoder.  2nd overwrites 1st
TMP_FP_CFILE=/tmp/fp_cfile.$$	# find parking color/legend config (as key=value
TMP_PFILE=/tmp/pspots.$$	# resolved addrs + sign cooords
TMP_CFILE=/tmp/cfile.$$		#
TMP_FP_CFILE_JSON=/tmp/json_file.$$	#

GEO=geo
GEO_2=ocd
VERBOSE=
DIST=
ADDR=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-v)
		VERBOSE="yes"
		shift
		;;
	-d)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-d requires distance arg (in feet)"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		DIST=$1
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

if [ ! -z "$DIST" ] ; then
	DIST="-d $DIST"
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
	ADDR=$FILE
fi

# try geocoder $GEO
$DM_SCRIPTS/get_geo_for_addrs.sh -d 0 -geo $GEO $AOPT "$ADDR" > $TMP_OFILE 2> $TMP_EFILE
n_OFILE=$(cat $TMP_OFILE | wc -l)
n_EFILE=$(grep ERROR: $TMP_EFILE | wc -l)
n_ADDRS=$((n_OFILE + n_EFILE))
if [ $n_EFILE -ne 0 ] ; then
	LOG ERROR "geocoder $GEO found $n_OFILE/$n_ADDRS addresses"
	grep '^ERROR' $TMP_EFILE | awk -F':' '{ print $4 }' > $TMP_AFILE_2
	n_ADDRS_2=$(cat $TMP_AFILE_2 | wc -l)
	$DM_SCRIPTS/get_geo_for_addrs.sh -geo $GEO_2 $TMP_AFILE_2 > $TMP_OFILE_2 2> $TMP_EFILE
	n_OFILE_2=$(cat $TMP_OFILE_2 | wc -l)
	n_EFILE=$(grep ERROR: $TMP_EFILE | wc -l)
	if [ $n_EFILE -ne 0 ] ; then
		LOG ERROR "backup geocoder $GEO_2 found an addtional $n_OFILE_2/$n_ADDRS_2 addresses"
		if [ "$VERBOSE" != "" ] ; then
			cat $TMP_EFILE 1>&2
		fi
	fi
	cat $TMP_OFILE_2 >> $TMP_OFILE
	n_OFILE=$(cat $TMP_OFILE | wc -l)
fi

# map address (if any)
if [ $n_OFILE -ne 0 ] ; then

	# very simple config that depends on number of addresses
	awk 'BEGIN {
		n_addrs = "'"$n_OFILE"'" + 0
	}
	END {
		printf("main.scale_type = factor\n")
		printf("main.values = 0.90,0.90,0.90 | 0.94,0.94,0.5 | 0.94,0.75,0.5 | 0.94,0.5,0.5%s\n",
			n_addrs > 1 ? " | 0.7,0.9,0.7" : "")
		printf("main.keys = PPL | PLU | PCVL | PTRKL%s\n", n_addrs > 1 ? " | rest" : "")
		printf("main.def_value = %s\n", "0.63,0.63,0.94")
		printf("main.def_key_text = dest\n")
	}' < /dev/null > $TMP_FP_CFILE

	$WM_BIN/find_addrs_in_rect $DIST -a $WM_DATA/sps_sorted.tsv $TMP_OFILE > $TMP_PFILE
	$AWK -F'\t' '
	@include '"$CFG_UTILS"'
	@include '"$INTERP_UTILS"'
	@include '"$COLOR_UTILS"'
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
		comma = index($NF, ",")
		key = comma > 0 ? substr($NF, 1, comma - 1) : $NF
		iv = IU_interpolate(color, key)
		printf("#%s\n", CU_rgb_to_24bit_color(iv))
	}' $TMP_PFILE > $TMP_CFILE
	$DM_SCRIPTS/cfg_to_json.sh $TMP_FP_CFILE > $TMP_FP_CFILE_JSON
	$DM_SCRIPTS/map_addrs.sh -sc $TMP_FP_CFILE_JSON -cf $TMP_CFILE -at src $TMP_PFILE
fi

rm -f $TMP_AFILE $TMP_AFILE_2 $TMP_FP_CFILE $TMP_OFILE $TMP_OFILE_2 $TMP_EFILE $TMP_PFILE $TMP_CFILE $TMP_FP_CFILE_JSON
