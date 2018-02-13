#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -mk merge-key -b base-tsv-file [ upd-tsv-file ]"

MKEY=
BFILE=
FILE=

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
	-b)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-b requires base-tsv-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		BFILE=$1
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

if [ -z "$MKEY" ] ; then
	LOG ERROR "missing -mk merge-key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$BFILE" ] ; then
	LOG ERROR "missing -b base-tsv-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' 'BEGIN {
	mkey = "'"$MKEY"'"
	bfile = "'"$BFILE"'"
	f_mkey = 0
	for(nb_ftab = nb_lines = nb_tab = 0; (getline < bfile) > 0; ){
		nb_lines++
		if(nb_lines == 1){	# header
			b_hdr = $0
			nb_ftab = NF;
			for(i = 1; i <= NF; i++){
				b_ftab[$i] = i
				if($i == mkey)
					f_mkey = i
			}
			if(f_mkey == 0){
				printf("ERROR: BEGIN: merge-key field %s not in base-file %s\n", mkey, bfile) > "/dev/stderr"
				err = 1
				break
			}
		}else{
			nb_tab++
			b_idx[$f_mkey] = nb_tab
			b_tab[nb_tab] = $0
		}
	}
	close(bfile)
	if(err)
		exit err
}
NR == 1 {
	f_mkey = 0
	nu_ftab = NF
	for(i = 1; i <= NF; i++){
		u_ftab[i] = $i
		if($i == mkey)
			f_mkey = i
	}
	if(f_mkey == 0){
		printf("ERROR: main: merge-key field %s not in update file %s\n", mkey, FILENAME == "" ? "__stdin__" : FILENAME) > "/dev/stderr"
		err = 1
		exit err
	}
	if(nu_ftab < 2){
		print("ERROR: main: update file %s contains only the merge-key %s data\n", FILENAME == "" ? "__stdin__" : FILENAME, mkey)
		err = 1
		exit err
	}
	for(i = 1; i <= nu_ftab; i++){
		if(i == f_mkey)
			continue
		if(!(u_ftab[i] in b_ftab)){
			printf("ERROR: main: update field %s not in base-file %s\n", u_ftab[i], bfile) > "/dev/stderr"
			err = 1
		}
	}
	if(err)
		exit err
}
NR > 1 {
	if(!($f_mkey in b_idx)){
		printf("ERROR: main: line %7d: merge-key %s not in base-file %s\n", $f_mkey, bfile) > "/dev/stderr"
		err = 1
		exit err
	}
	b_row = b_tab[b_idx[$f_mkey]]
	nb_ary = split(b_row, b_ary)
	for(i = 1; i <= NF; i++){
		if(i == f_mkey)
			continue
		b_ary[b_ftab[u_ftab[i]]] = $i
	}
	b_line = b_ary[1]
	for(i = 2; i <= nb_ary; i++)
		b_line = b_line "\t" b_ary[i]
	b_tab[b_idx[$f_mkey]] = b_line
}
END {
	if(err)
		exit err
	printf("%s\n", b_hdr) > bfile
	for(i = 1; i <= nb_tab; i++)
		printf("%s\n", b_tab[i]) > bfile
	close(bfile)
}' $FILE
