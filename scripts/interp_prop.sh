#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -c cfg-file -bk base-key -nk new-key [ -eeta meta-file ] [ prop-file ]"

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOE is not defined"
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
elif [ "$AWK_VERSION" == "4" ] ; then
	AWK=awk
	CFG_UTILS="\"$DM_LIB/cfg_utils.awk\""
	INTERP_UTILS="\"$DM_LIB/interp_utils.awk\""
else
	LOG ERROR "unsupported awk version: \"$AWK_VERSION\": must be 3 or 4"
	exit 1
fi

CFILE=
BKEY=
NKEY=
MFILE=
FILE=

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
	-bk)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-bk requires base-key argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		BKEY=$1
		shift
		;;
	-nk)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-nk requires base-key argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		NKEY=$1
		shift
		;;
	-meta)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-meta requires meta-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		MFILE=$1
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

if [ -z "$CFILE" ] ; then
	LOG ERROR "missing -c cfg.file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$BKEY" ] ; then
	LOG ERROR "missing -bk base-key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$NKEY" ] ; then
	LOG ERROR "missing -nk new-key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

$AWK -F'\t' '
@include '"$CFG_UTILS"'
@include '"$INTERP_UTILS"'
BEGIN {
	pr_hdr = 1
	cfile = "'"$CFILE"'"
	bkey = "'"$BKEY"'"
	nkey = "'"$NKEY"'"
	mfile = "'"$MFILE"'"
	if(CFG_read(cfile, config)){
		err = 1
		exit err
	}
	iv_name = nkey ".values"
	if(!(("_globals", iv_name) in config)){
		printf("ERROR: BEGIN: no interp data for %s in %s\n", nkey, cfile) > "/dev/stderr"
		err = 1
		exit err
	}
	if(IU_init(config, interp, nkey)){
		err = 1
		exit err
	}
	g_min_max = interp["scale_type"] != "factor"
	mm_init = 0

	# IU_dump("/dev/stderr", interp)
}
NR == 1 {
	f_bkey = -1
	for(i = 1; i <= NF; i++){
		if($i == bkey)
			f_bkey = i
	}
	if(f_bkey == -1){
		printf("ERROR: BEGIN: base key %s not in prop-file header\n", bkey) > "/dev/stderr"
		err = 1
		exit err
	}
	hdr = $0
}
NR > 1 {
	if(pr_hdr){
		pr_hdr = 0
		printf("%s", hdr)
		printf("\t%s\n", nkey)
	}
	# get min/max for non factor interpolation
	if(g_min_max){
		if(!mm_init){
			mm_init = 1
			bK_min = bk_max = $f_bkey
		}
		if($f_bkey < bk_min)
			bk_min = $f_bkey
		if($f_bkey > bk_max)
			bk_max = $f_bkey
	}
	printf("%s", $0)
	printf("\t%s\n", IU_interpolate(interp, $f_bkey))
}
END {
	if(err)
		exit err;
	if(mfile){
		printf("%s.min_value = %g\n", interp["name"], bk_min) >> mfile
		printf("%s.max_value = %g\n", interp["name"], bk_max) >> mfile

		# BEGIN orig
		printf("%s.stats = %d,%.1f", interp["name"], interp["counts", 1], 100.0*interp["counts", 1]/interp["tcounts"]) >> mfile
		for(i = 2; i <= interp["nbreaks"] + 1; i++)
			printf(" | %d,%.1f", interp["counts", i], 100.0*interp["counts", i]/interp["tcounts"]) >> mfile
		printf("\n") >> mfile
		# END orig

		# BEGIN new, not yet used
		printf("%s.counts = %d", interp["name"], interp["counts", 1]) >> mfile
		for(i = 2; i <= interp["nbreaks"] + 1; i++)
			printf(" | %d", interp["counts", i]) >> mfile
		printf("\n") >> mfile
		printf("%s.pcts = %.1f", interp["name"], 100.0*interp["counts", 1]/interp["tcounts"]) >> mfile
		for(i = 2; i <= interp["nbreaks"] + 1; i++)
			printf(" | %.1f", 100.0*interp["counts", i]/interp["tcounts"]) >> mfile
		printf("\n") >> mfile
		# END new, not yet used

		close(mfile)
	}
	exit 0
}' $FILE
