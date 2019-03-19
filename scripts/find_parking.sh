#  /bin/bash
#
. ~/etc/funcs.sh
U_MSG="usage: $0 [ -help ] [ -dt date_time ] [ -log { file | NONE } ] [ -d D ] [ -app { gh*|dd|pm|ue } ] { -a address | [ address-file ] }"

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

FP_DATA=$HOME/work/trip_data/seattle/fp_data

TMP_OFILE=/tmp/out.$$		# output of 1st geocoder
TMP_EFILE=/tmp/err.$$		# merged errs for all geocoders
TMP_FP_CFILE=/tmp/fp_cfile.$$	# find parking color/legend config (as key=value
TMP_PFILE=/tmp/pspots.$$	# resolved addrs + sign cooords
TMP_CFILE=/tmp/cfile.$$		# color file for points
TMP_FP_CFILE_JSON=/tmp/json_file.$$	# json version of TMP_FP_CFILE.$$

# save the args so they can be logged, if requested
ARGS="$@"
LOG_DT=
LOG=
DIST=
APP="gh"
ADDR=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-dt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-dt requires date_time argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		LOG_DT=$1
		shift
		;;
	-log)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-log requires file arg or NONE"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		LOG=$1
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

# allow user to set the date/time.  Intended for data preceding this script
if [ -z "$LOG_DT" ] ; then
	LOG_DT="$NOW"
fi

# process LOG arg
if [ -z "$LOG" ] ; then	# LOG="", use default log dir & log file
	LOG_DIR=$FP_DATA/"$(echo $LOG_DT | awk -F_ '{ printf("%s/%s", substr($1, 1, 4),  substr($1, 1, 6)) }')"
	if [ ! -d $LOG_DIR ] ; then
		emsg="$(mkdir -p $LOG_DIR 2>&1)"
		if [ ! -z "$emsg" ] ; then
			LOG INFO $emsg
			exit 1
		fi
	fi
	LOG_FILE=$LOG_DIR/"$(echo $LOG_DT | awk -F_ '{ printf("fp.%s.log",  $1) }')"
else
	LOG_FILE="$LOG"
fi

# if requested, log the command; since it's saved, maybe do more? later in the script?
if [ "$LOG_FILE" != "NONE" ] ; then
	echo "$LOG_DT $ARGS" >> $LOG_FILE
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

$DM_SCRIPTS/get_geo_for_addrs.sh -d 0 $AOPT "$ADDR" >> $TMP_OFILE 2> $TMP_EFILE
n_OFILE=$(cat $TMP_OFILE | wc -l)

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

if [ -s $TMP_EFILE ] ; then
	cat $TMP_EFILE 1>&2
fi

rm -f $TMP_FP_CFILE $TMP_OFILE $TMP_EFILE $TMP_PFILE $TMP_CFILE $TMP_FP_CFILE_JSON
