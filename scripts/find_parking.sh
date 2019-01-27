#  /bin/bash
#
. ~/etc/funcs.sh
U_MSG="usage: $0 [ -help ] [ -d D ] [ -gl gc-list ] [ -app { gh*|dd|pm|ue } ] { -a address | [ address-file ] }"

NOW="$(date +%Y%m%d_%H%M%S)"

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
DM_ETC=$DM_HOME/etc
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

. $DM_ETC/geocoder_defs.sh

# Need to save & date the calls to find_parking.sh, so here's v-0.0.1
FP_DATA=$HOME/fp_data
LOG_DIR=$FP_DATA/"$(echo $NOW | awk -F_ '{ printf("%s/%s", substr($1, 1, 4),  substr($1, 1, 6)) }')"
if [ ! -d $LOG_DIR ] ; then
	emsg="$(mkdir -p $LOG_DIR 2>&1)"
	if [ ! -z "$emsg" ] ; then
		LOG INFO $emsg
		exit 1
	fi
fi
LOG_FILE=$LOG_DIR/"$(echo $NOW | awk -F_ '{ printf("fp.%s.log",  $1) }')"
# do this _before_ arg processing or $@ will be empty
echo "$NOW $@" >> $LOG_FILE

TMP_AFILE=/tmp/addrs.$$		# addrs for 1st geocoder
TMP_AFILE_2=/tmp/addrs_2.$$	# addrs for 2nd geocoder
TMP_OFILE=/tmp/out.$$		# output of 1st geocoder
TMP_OFILE_2=/tmp/out_2.$$	# output of 2nd geocoder, eventually appended to TMP_OFILE
TMP_EFILE=/tmp/err.$$		# errs for 1st, 2nd geocoders
TMP_EFILE_1=/tmp/$GEO_1.err.$$	# errs for 1st geocoder
TMP_EFILE_2=/tmp/$GEO_2.err.$$	# errs for 2nd geocoder
TMP_FP_CFILE=/tmp/fp_cfile.$$	# find parking color/legend config (as key=value
TMP_PFILE=/tmp/pspots.$$	# resolved addrs + sign cooords
TMP_CFILE=/tmp/cfile.$$		# color file for points
TMP_FP_CFILE_JSON=/tmp/json_file.$$	# json version of TMP_FP_CFILE.$$

DIST=
GC_LIST=
APP="gh"
ADDR=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
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
	-gl)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-geo requires geo-list argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		GC_LIST=$1
		shift
		;;
	-app)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-app requires app-name argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		APP=$1
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

# set up the geocoder order
if [ -z "$GC_LIST" ] ; then
	GC_LIST="$GEO_PRIMARY,$GEO_SECONDARY"
	GEO_1=$GEO_PRIMARY
	GEO_2=$GEO_SECONDARY
else
	GC_WORK="$(chk_geocoders "$GC_LIST")"
	if echo "$GC_WORK" | grep '^ERROR' ; then
		LOG ERROR "$GC_WORK"
		exit 1
	fi
	GEO_1=$(echo "$GC_WORK" | awk -F, '{ print $1 }')
	GEO_2=$(echo "$GC_WORK" | awk -F, '{ print $2 }')
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

# try geocoder $GEO_1
$DM_SCRIPTS/get_geo_for_addrs.sh -d 0 -efmt new -geo $GEO_1 $AOPT "$ADDR" > $TMP_OFILE 2> $TMP_EFILE_1
n_OFILE=$(cat $TMP_OFILE | wc -l)
n_EFILE_1=$(grep '^ERROR' $TMP_EFILE_1 | wc -l)
n_ADDRS=$((n_OFILE + n_EFILE_1))
if [ $n_EFILE_1 -gt 0 ] ; then
	grep '^ERROR' $TMP_EFILE_1 | awk -F'\t' '{ print $4 }' > $TMP_AFILE_2
	n_ADDRS_2=$(cat $TMP_AFILE_2 | wc -l | tr -d ' ')
	# try geocoder $GEO_2
	$DM_SCRIPTS/get_geo_for_addrs.sh -d 0 -efmt new -geo $GEO_2 $TMP_AFILE_2 > $TMP_OFILE_2 2> $TMP_EFILE_2
	n_OFILE_2=$(cat $TMP_OFILE_2 | wc -l | tr -d ' ')
	n_EFILE_2=$(grep '^ERROR' $TMP_EFILE_2 | wc -l | tr -d ' ')
	if [ $n_EFILE_2 -gt 0 ] ; then
		LOG ERROR "$n_EFILE_2/$n_ADDRS addresses were not found"
		$DM_SCRIPTS/merge_geo_error_files.sh $TMP_EFILE_1 $TMP_EFILE_2 1>&2
	fi
	cat $TMP_OFILE_2 >> $TMP_OFILE
	n_OFILE=$(cat $TMP_OFILE | wc -l)
fi

# map address(es) (if any)
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
		if(key ~ /^rest:/)
			key = "rest"
		iv = IU_interpolate(color, key)
		printf("#%s\n", CU_rgb_to_24bit_color(iv))
	}' $TMP_PFILE > $TMP_CFILE
	$DM_SCRIPTS/cfg_to_json.sh $TMP_FP_CFILE > $TMP_FP_CFILE_JSON
	$DM_SCRIPTS/map_addrs.sh -sc $TMP_FP_CFILE_JSON -cf $TMP_CFILE -at src $TMP_PFILE
fi

rm -f $TMP_AFILE $TMP_AFILE_2 $TMP_FP_CFILE $TMP_OFILE $TMP_OFILE_2 $TMP_EFILE $TMP_EFILE_1 $TMP_EFILE_2 $TMP_PFILE $TMP_CFILE $TMP_FP_CFILE_JSON
