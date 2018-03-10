#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -trace ] -mk merge-key [ -pfx_of target-field ] [ -mv missing-values ] file-1 file-2 [ file-3 ... ]"

TRACE=
MKEY=
PFX_OF=
MISSING=
FLIST=
n_FLIST=0

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-trace)
		TRACE="yes"
		shift
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
	-pfx_of)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-pfx_of requires target-field argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		PFX_OF="$1"
		shift
		;;
	-mv)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-mv requires missing values argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		MISSING="$1"
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
	trace = "'"$TRACE"'" == "yes"
	if(trace)
		printf("TRACE: BEGIN: add columns\n") > "/dev/stderr"
	mkey = "'"$MKEY"'"
	pfx_of = "'"$PFX_OF"'"
	missing = "'"$MISSING"'"
	if(missing != ""){
		nm_ary = split(missing, m_ary)
		for(i = 1; i <= nm_ary; i++)
			printf("m_ary[%d] = \"%s\"\n", i, m_ary[i]) > "/dev/stderr"
	}
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
			if(pfx_of != ""){
				f_pfx_of = fi_key(pfx_of)
				if(f_pfx_of == 0){
					printf("ERROR: main: no field named %s in file %s\n", pfx_of, FILENAME) > "/dev/stderr"
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
			if(f_pfx_of != 0){
				if($f_mkey ~ /\/$/){
					r_idx_pfx_of[$f_pfx_of] = n_recs
					if(!($f_mkey in p_idx)){
						n_pfx++
						p_idx[$f_mkey] = n_pfx
						pfx_targets[n_pfx] = $f_pfx_of
						pfx_was_set[n_pfx] = ""
						pfx_values[n_pfx] = ""
					}else	
						pfx_targets[p_idx[$f_mkey]] = pfx_targets[p_idx[$f_mkey]] "|" $f_pfx_of
				}
			}
		}
	}else if(lnum == 1){	# hdr for file 2,...
		if(fnum > 2){
			if(pfx_of == ""){
#				if(n_recs != n_recs2){
#					printf("ERROR: main: num recs differ: file-1 %s, %d recs, file-%d %s, %d recs\n",
#						fname_1, n_recs, fnum - 1, l_FILENAME, n_recs2) > "/dev/stderr"
#					err = 1
#					exit err
#				}
				if(n_recs2 > n_recs){
					printf("ERROR: main: %d extra recs in file-%d, %s\n", n_recs2 - n_recs, fnum - 1, l_FILENAME) > "/dev/stderr"
					err = 1
					exit err
				}else if(n_recs > n_recs2){
					if(missing == ""){
						printf("ERROR: END: missing recs in file-%d, %s, requires -mv missing values argument\n", fnum - 1, l_FILENAME) > "/dev/stderr"
						err = 1
						goto CLEAN_UP
					}
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
				if(f_pfx_of != 0 && $f_mkey ~ /\/$/){
					nf = split(recs[r_idx[$f_mkey]], ary, "\t")
					pfx_was_set[p_idx[$f_mkey]] = ary[f_pfx_of]
					pfx_values[p_idx[$f_mkey]] = (pfx_values[p_idx[$f_mkey]] != "") ? (pfx_values[p_idx[$f_mkey]] "\t" $i) : $i
				}
			}
		}
	}
	l_FILENAME = FILENAME
}
END {
	if(err){
		if(trace)
			printf("TRACE: END: add columns had error(s)\n") > "/dev/stderr"
		exit err
	}

	#dump_pfx_info("/dev/stderr", p_idx, pfx_targets, pfx_was_set, pfx_values)

	if(!pfx_of){
#		if(n_recs != n_recs2){
#			printf("ERROR: END: num recs differ: file-1 %s, %d recs, file-%d %s, %d recs\n",
#				fname_1, n_recs, fnum, l_FILENAME, n_recs2) > "/dev/stderr"
#			err = 1
#			exit err
#		}
		if(n_recs2 > n_recs){
			printf("ERROR: END: %d extra recs in file-%d, %s\n", n_recs2 - n_recs, fnum - 1, l_FILENAME) > "/dev/stderr"
			err = 1
			exit err
		}else if(n_recs > n_recs2){
			if(missing == ""){
				printf("ERROR: END: missing recs in file-%d, %s, requires -mv missing values argument\n", fnum - 1, l_FILENAME) > "/dev/stderr"
				err = 1
				goto CLEAN_UP
			}
		}
	}
	for(k in have_data){
		if(!have_data[k]){
			printf("ERROR: END: file-%d, %s: no data for %s\n", fnum, l_FILENAME, k) > "/dev/stderr"
			err = 1
			exit err
		}
	}

	if(f_pfx_of != 0){
		fill_pfx_values(p_idx, pfx_targets, pfx_was_set, pfx_values, r_idx_pfx_of, recs)
	}

	printf("%s\n", hdr)
	for(i = 1; i <= n_recs; i++)
		printf("%s\n", recs[i])

	if(trace)
		printf("TRACE: END: add columns\n") > "/dev/stderr"

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
}
function fill_pfx_values(p_idx, pfx_targets, pfx_was_set, pfx_values, r_idx_pfx_of, recs,   k, nt, t_ary, i, t_ws) {

	for(k in p_idx){
		ws = pfx_was_set[p_idx[k]]
		t_v = pfx_values[p_idx[k]]
		nt = split(pfx_targets[p_idx[k]], t_ary, "|")
		for(t = 1; t <= nt; t++){
			if(t_ary[t] != ws)
				recs[r_idx_pfx_of[t_ary[t]]] = recs[r_idx_pfx_of[t_ary[t]]] "\t" t_v
		}
	}
}
function dump_pfx_info(file, p_idx, pfx_targets, pfx_was_set, pfx_values,   k, ary) {

	for(k in p_idx){
		printf("%s = {\n", k) > file
		printf("\ttargets = %s\n", pfx_targets[p_idx[k]]) > file
		printf("\twas_set = %s\n", pfx_was_set[p_idx[k]]) > file
		printf("\tvalues  = %s\n", pfx_values[p_idx[k]]) > file
		printf("}\n") > file
	}
}' $FLIST

