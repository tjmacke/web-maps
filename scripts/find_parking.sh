#  /bin/bash
#
. ~/etc/funcs.sh
U_MSG="usage: $0 [ -help ] [ -dt date_time ] [ -sn ] [ -gl gc-list ] [ -log NONE ] [ -last N ] [ -d D ] [ -app { gh*|dd|pm|ue } ] { -a address | [ address-file ] }"

NOW="$(date +%Y%m%d_%H%M%S)"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data/Seattle_Parking
WM_SRC=$WM_HOME/src
WM_SCRIPTS=$WM_HOME/scripts
SN_FLIST=$WM_DATA/sn.list.json

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME not defined"
	exit 1
fi
DM_ETC=$DM_HOME/etc
DM_LIB=$DM_HOME/lib
DM_SCRIPTS=$DM_HOME/scripts

. $DM_ETC/geocoder_defs.sh

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

FP_LOGS=$TD_HOME/seattle/fp_logs
FP_DATA=$TD_HOME/seattle/fp_data
FP_DB=$FP_DATA/trips.db
if [ ! -s $FP_DB ] ; then
	LOG ERROR "database $FP_DB either does not exist or has zero size"
	exit 1
fi

TMP_AFILE=/tmp/addrs.$$		# all addresseses including last if available
TMP_OFILE=/tmp/out.$$		# output of all geocoders
TMP_EFILE=/tmp/err.$$		# merged errs for all geocoders
TMP_FP_CFILE=/tmp/fp_cfile.$$	# find parking color/legend config (as key=value
TMP_PFILE=/tmp/pspots.$$	# resolved addrs + sign cooords
TMP_CFILE=/tmp/cfile.$$		# color file for points
TMP_FP_CFILE_JSON=/tmp/json_file.$$	# json version of TMP_FP_CFILE.$$

# save the args so they can be logged, if requested
ARGS="$@"
LOG_DT=
SN=
GC_LIST=
LOG=
LAST=
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
	-sn)
		SN="yes"
		shift
		;;
	-gl)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-gl requries gc-list argumenet"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		GC_LIST=$1
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
	-last)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-lasts requires integer argument"
			echo "$U_UMSG" 1>&2
			exit 1
		fi
		LAST=$1
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

# process LOG arg, LOG_FILE is always LOG_DIR/fp.${LOG_DT}.log, but if LOG == NONE the current cmd is not logged
LOG_DIR=$FP_LOGS/"$(echo $LOG_DT | awk -F_ '{ printf("%s/%s", substr($1, 1, 4),  substr($1, 1, 6)) }')"
if [ ! -d $LOG_DIR ] ; then
	emsg="$(mkdir -p $LOG_DIR 2>&1)"
	if [ ! -z "$emsg" ] ; then
		LOG INFO $emsg
		exit 1
	fi
fi
LOG_FILE=$LOG_DIR/"$(echo $LOG_DT | awk -F_ '{ printf("fp.%s.log",  $1) }')"

# set up the geocoder order
if [ -z "$GC_LIST" ] ; then
	GC_LIST="$GEO_PRIMARY,$GEO_SECONDARY"
else
	GC_WORK="$(chk_geocoders $GC_LIST)"
	if echo "$GC_WORK" | grep '^ERROR' > /dev/null ; then
		LOG ERROR "$GC_WORK"
		exit 1
	fi
	GC_LIST=$GC_WORK	# comma sep list w/o spaces
fi

# get last address if it exists
if [ -f $LOG_FILE ] ; then
	LAST_ADDR="$(tail -1 $LOG_FILE |
		awk 'BEGIN {
			last = "'"$LAST"'"
		}
		{
			ix = index($0, "-a")
			alist = substr($0, ix+3)
			n_ary = split(alist, ary, "|")
			for(i = 2; i <= n_ary; i++){
				sub(/^  */, "", ary[i])
				sub(/  *$/, "", ary[i])
			}
			if(last == "")
				last_addr = ary[n_ary]
			else{
				last += 0
				last_addr = ""
				for(i = 2; i <= n_ary; i++){
					ix = index(ary[i], ".")
					anum = substr(ary[i], 1, ix-1)
					if(anum == last){
						last_addr = ary[i]
						break
					}
				}
			}
			if(last_addr == ""){
				printf("ERROR: END: no addr %d in %s\n", last, alist) > "/dev/stderr"
			}else{
				sub(/^[1-9]\.  */, "last: ", last_addr)
			}
			print last_addr
		}')"
fi

# log the cmd if requested
if [ "$LOG" != "NONE" ] ; then
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
	echo "$ADDR" > $TMP_AFILE
else
	cat $FILE > $TMP_AFILE
fi
if [ ! -z "$LAST_ADDR" ] ; then
	echo "$LAST_ADDR" >> $TMP_AFILE
fi

# Parse address lines and check if they are in FP_DB or the soon to be implemented.
# cache. This insures that only addresses w/o (lng, lat) are sent to a geocoder.
cat $TMP_AFILE |
while read line ; do
	echo "$line" |
	awk -F'|' '{
		# split on pipe into an array of possibly prefixed addresses
		# trim leading/trailing spaces
		# find addr prefixes (if any)
		for(i = 1; i <= NF; i++){
			sub(/^  */, "", $i)
			sub(/  *$/, "", $i)
			if(match($i, /^rest:  */)){
				pfx = substr($i, 1, RLENGTH)
				addr = substr($i, RLENGTH + 1)
			}else if(match($i, /^[1-9]\.  */)){
				pfx = substr($i, 1, RLENGTH)
				addr = substr($i, RLENGTH + 1)
			}else if(match($i, /^last: */)){
				pfx = substr($i, 1, RLENGTH)
				addr = substr($i, RLENGTH + 1)
			}else{	# no pfx
				pfx = ""
				addr = $i
			}
			printf("%s\t%s\n", pfx, addr)
		}
	}'	|
	while read pa_line ; do
		pfx="$(echo "$pa_line" | awk -F'\t' '{ print $1 }')"
		addr="$(echo "$pa_line" | awk -F'\t' '{ print $2 }')"
		gc_addr="$($WM_SCRIPTS/select_gc_addr_from_db.sh  -db $FP_DB "$addr")"
		# add any prefix
		if [ ! -z "$gc_addr" ] ; then
			echo "$pfx|$gc_addr" | awk -F'|' '{
				if($1 == "")
					print $2
				else{
					n_ary = split($2, ary, "\t")
					ary[2] = $1 ary[2]
					printf("%s", ary[1])
					for(i = 2; i <= n_ary; i++)
						printf("\t%s", ary[i])
					printf("\n")
				}
			}' 1>&2
		else
			echo "Not in DB: $addr, chk cache" 1>&2
		fi
	done
done

$DM_SCRIPTS/get_geo_for_addrs.sh -d 1 -gl $GC_LIST $TMP_AFILE > $TMP_OFILE 2> $TMP_EFILE
n_OFILE=$(cat $TMP_OFILE | wc -l)
# map address(es) (if any)
if [ $n_OFILE -ne 0 ] ; then
	h_REST="$(awk '$1 == "rest:" { yes = 1 ; exit 0 } END { print yes ? "yes" : "" }' $TMP_AFILE)"
	h_LAST="$(awk '$1 == "last:" { yes = 1 ; exit 0 } END { print yes ? "yes" : "" }' $TMP_AFILE)"

	# very simple config that depends on number of addresses
	awk 'BEGIN {
		h_rest = "'"$h_REST"'" == "yes"
		h_last = "'"$h_LAST"'" == "yes"
	}
	END {
		printf("main.scale_type = factor\n")
		printf("main.values = 0.90,0.90,0.90 | 0.94,0.94,0.5 | 0.94,0.75,0.5 | 0.94,0.5,0.5%s%s\n",
			h_last ? " | 0.65,0.65,0.65" : "",
			h_rest ? " | 0.7,0.9,0.7" : "")
		printf("main.keys = PPL | PLU | PCVL | PTRKL%s%s\n", h_last ? "| last" : "", h_rest ? " | rest" : "")
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
		else if(key ~ /^last:/)
			key = "last"
		iv = IU_interpolate(color, key)
		printf("#%s\n", CU_rgb_to_24bit_color(iv))
	}' $TMP_PFILE > $TMP_CFILE
	$DM_SCRIPTS/cfg_to_json.sh $TMP_FP_CFILE > $TMP_FP_CFILE_JSON
	if [ "$SN" == "yes" ] ; then
		$DM_SCRIPTS/map_addrs.sh -sc $TMP_FP_CFILE_JSON -cf $TMP_CFILE -fl $SN_FLIST -at src $TMP_PFILE
	else
		$DM_SCRIPTS/map_addrs.sh -sc $TMP_FP_CFILE_JSON -cf $TMP_CFILE -at src $TMP_PFILE
	fi
fi

if [ -s $TMP_EFILE ] ; then
	cat $TMP_EFILE 1>&2
fi

rm -f $TMP_AFILE $TMP_OFILE $TMP_EFILE $TMP_FP_CFILE $TMP_PFILE $TMP_CFILE $TMP_FP_CFILE_JSON
