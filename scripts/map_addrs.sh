#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -sc scale-conf-file ] [ -p prop-file [ -mk merge-key] ] [ -fl flist-json ] -at { src | dst } [ addr-geo-file ]"

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME is not defined"
	exit 1
fi
DM_ETC=$DM_HOME/etc
DM_LIB=$DM_HOME/lib
DM_SCRIPTS=$DM_HOME/scripts

# awk v3 does not support include
AWK_VERSION="$(awk --version | awk '{ nf = split($3, ary, /[,.]/) ; print ary[1] ; exit 0 }')"
if [ "$AWK_VERSION" == "3" ] ; then
	AWK="igawk --re-interval"
	GEO_UTILS="$DM_LIB/geo_utils.awk"
elif [ "$AWK_VERSION" == "4" ] ; then
	AWK=awk
	GEO_UTILS="\"$DM_LIB/geo_utils.awk\""
else
	LOG ERROR "unsupported awk version \"$AWK_VERSION\", must be 3 or 4"
	exit 1
fi

SC_CFG=
PFILE=
MKEY=
FLIST=
ATYPE=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-sc)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-c requires scale-config-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SC_CFG=$1
		shift
		;;
	-p)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-p requires prop-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		PFILE=$1
		shift
		;;
	-mk)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-mk requires merge-key argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		MKEY=$1
		shift
		;;
	-fl)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-fl requires flist-json argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FLIST="$1"
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

if [ -z "$ATYPE" ] ; then
	LOG ERROR "missing -at address-type argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ "$ATYPE" != "src" ] && [ "$ATYPE" != "dst" ] ; then
	LOG ERROR "unknown address type $ATYPE, must src or dst"
	echo "$U_MSG" 1>&2
			exit 1
	fi

if [ ! -z "PFILE" ] ; then
	if [ -z "$MKEY" ] ; then
		MKEY=addr
	fi
elif [ ! -z "$MKEY" ] ; then
	LOG ERROR "non empty MKEY requires -p prop-file argument"
	echo "$U_MSG" 1>&2
	exit 1

fi

sort -k 4g,4 -k 5g,5 $FILE	|
$AWK -F'\t' '
@include '"$GEO_UTILS"'
BEGIN {
	prog = "'"$PROG"'"
	pfile = "'"$PFILE"'"
	mkey = "'"$MKEY"'"
	if(pfile != ""){
		# read props
		f_mkey = 0
		for(n_ptab = n_plines = 0; (getline < pfile) > 0; ){
			n_plines++
			if(n_plines == 1){	#header
				np_ftab = NF
				for(i = 1; i <= NF; i++){
					p_ftab[$i] = i
					if($i == mkey)
						f_mkey = i
				}
				if(f_mkey == 0){
					printf("ERROR: BEGIN: merge-key field %s not in prop-file %s\n", mkey, pfile) > "/dev/stderr"
					err = 1
					break
				}
			}else{
				n_ptab++
				p_idx[$f_mkey] = i
				ptab[n_ptab] = $0
			}
		}
		close(pfile)
		if(err)
			exit err
	}else
		n_ptab = 0
	flist = "'"$FLIST"'"
	atype = "'"$ATYPE"'"
	f_addr = atype == "src" ? 2 : 3
}
{
	n_addrs++
	date[n_addrs] = $1
	s_addr[n_addrs] = $2
	d_addr[n_addrs] = $3
	lng[n_addrs] = $4
	lat[n_addrs] = $5
	rply_addr[n_addrs] = $6
}
END {
	if(err)
		exit err

	if(n_addrs == 0)
		exit 0

	meta_data["count"] = 1
	meta_data[meta_data["count"], "key"] = "createdBy"
	meta_data[meta_data["count"], "value"] = prog
	meta_data[meta_data["count"], "is_str"] = 1
	meta_data["count"] = 2
	meta_data[meta_data["count"], "key"] = "n_features"
	meta_data[meta_data["count"], "value"] = n_points
	meta_data[meta_data["count"], "is_str"] = 0
	GU_pr_header(sc_cfg, meta_data)

	# add any other features
	if(flist != ""){
		n_of = 0
		while((getline line < flist) > 0){
			n_of++
			printf("%s\n", line)
		}
		if(n_of > 0)
			printf(",\n")
		close(flist)
	}

	n_pgroups = GU_find_pgroups(1, n_addrs, lng, lat, pg_starts, pg_counts)
	for(i = 1; i <= n_pgroups; i++){
		GU_geo_adjust(lng[pg_starts[i]], lat[pg_starts[i]], pg_counts[i], lng_adj, lat_adj)
		for(j = 0; j < pg_counts[i]; j++){
			s_idx = pg_starts[i] + j
			GU_mk_point("/dev/stdout",
				colors[s_idx], styles[s_idx], lng[s_idx] + lng_adj[j+1], lat[s_idx] + lat_adj[j+1], titles[s_idx], (s_idx == n_points))
		}
	}

	GU_pr_trailer()
	exit 0
}'
rval=$?
exit $rval
