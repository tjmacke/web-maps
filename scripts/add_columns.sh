#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -mk merge-key [ -pfx target-field ] file-1 file-2 [ file-3 ... ]"

MKEY=
PFX=
FLIST=
n_FLIST=0

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-mk)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-mk requires merge-key argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		MKEY="$1"
		shift
		;;
	-pfx)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-pfx requires target-field argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		PFX="$1"
		shift
		;;
	-*)
		LOG ERROR "unknown argument $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		if [ ! -z "$FLIST" ] ; then
			FLIST="$FLIST "
		fi
		FLIST="$FLIST$1"
		n_FLIST=$((n_FLIST + 1))
		shift
		;;
	esac
done

if [ -z "$MKEY" ] ; then
	LOG ERROR "missing -mk merge-key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ $n_FLIST -lt 2 ] ; then
	LOG ERROR "not enough files: $n_FLIST, need at least 2"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' 'BEGIN {
	mkey = "'"$MKEY"'"
	pfx = "'"$PFX"'"
}
{
	if(l_FILENAME != FILENAME){
		fnum++
		lnum = 0
	}
	lnum++
	if(fnum == 1){
		if(lnum == 1){	# hdr for file 1
			f_mkey = fi_key(mkey)
			if(f_mkey == 0){
				printf("ERROR: main: no field named %s in file %s\n", mkey, FILENAME) > "/dev/stderr"
				err = 1
				exit err
			}
			if(pfx != ""){
				f_pfx = fi_key(pfx)
				if(f_pfx == 0){
					printf("ERROR: main: no field named %s in file %s\n", key, FILENAME) > "/dev/stderr"
					err = 1
					exit err
				}
			}
			fname_1 = FILENAME
			hdr = $0
		}else{		# data for file 1
			n_recs++
			r_idx[$f_mkey] = n_recs
			recs[n_recs] = $0
			if(f_pfx != 0){
				if($f_mkey ~ /\/$/)
					pfx_tgts[$f_mkey] = ($f_mkey in pfx_tgts) ? (pfx_tgts[$f_mkey] "|" $f_pfx) : $f_pfx
			}
		}
	}else if(lnum == 1){	# hdr for file 2,...
		if(fnum > 2){
			if(pfx == ""){
				if(n_recs != n_recs2){
					printf("ERROR: main: num recs differ: file-1 %s, %d recs, file-%d %s, %d recs\n",
						fname_1, n_recs, fnum - 1, l_FILENAME, n_recs2) > "/dev/stderr"
					err = 1
					exit err
				}
			}
			for(k in have_data){
				if(!have_data[k]){
					printf("ERROR: main: file-%d, %s: no data for %s\n", fnum - 1, l_FILENAME, k) > "/dev/stderr"
					err = 1
					exit err
				}
			}
		}
		f_mkey = fi_key(mkey)
		if(f_mkey == 0){
			printf("ERROR: main: no field named %s in file %s\n", mkey, FILENAME) > "/dev/stderr"
			err = 1
			exit err
		}
		n_recs2 = 0
		for(k in r_idx)
			have_data[k] = 0
		for(i = 1; i <= NF; i++){
			if(i != f_mkey)
				hdr = hdr "\t" $i
		}
	}else{			# data for file 2,...
		n_recs2++
		have_data[$f_mkey] = 1
		for(i = 1; i <= NF; i++){
			if(i != f_mkey){
				recs[r_idx[$f_mkey]] = recs[r_idx[$f_mkey]] "\t" $i
			}
		}
	}
	l_FILENAME = FILENAME
}
END {
	if(err)
		exit err

	if(!pfx){
		if(n_recs != n_recs2){
			printf("ERROR: END: num recs differ: file-1 %s, %d recs, file-%d %s, %d recs\n",
				fname_1, n_recs, fnum, l_FILENAME, n_recs2) > "/dev/stderr"
			err = 1
			exit err
		}
	}
	for(k in have_data){
		if(!have_data[k]){
			printf("ERROR: END: file-%d, %s: no data for %s\n", fnum - 1, l_FILENAME, k) > "/dev/stderr"
			err = 1
			exit err
		}
	}

	printf("%s\n", hdr)
	for(i = 1; i <= n_recs; i++)
		printf("%s\n", recs[i])

	exit err
}
function fi_key(key,   f_key, i) {
	
	f_key = 0
	for(i = 1; i <= NF; i++){
		if($i == key){
			f_key = i
			break
		}
	}
	return f_key
}' $FLIST

