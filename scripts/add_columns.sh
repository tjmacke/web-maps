#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] -b base-file -mk merge-key [ prop-file ]"

BFILE=
MKEY=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-b)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-b requires base-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		BFILE=$1
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

if [ -z "$BFILE" ] ; then
	LOG ERROR "missing -b base-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$MKEY" ] ; then
	LOG ERROR "missing -mk merge-key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' 'BEGIN {
	bfile = "'"$BFILE"'"
	mkey = "'"$MKEY"'"
	for(n_blines = n_btab = 0; (getline < bfile) > 0; ){
		n_blines++
		if(n_blines == 1){
			bhdr = $0
			b_mkey = -1
			for(i = 1; i <= NF; i++){
				if($i == mkey){
					b_mkey = i
					break
				}
			}
			if(b_mkey == -1){
				printf("ERROR: BEGIN: mkey \"%s\" is not in hdr of bfile \"%s\"\n", mkey, bfile) > "/dev/stderr"
				err = 1
				exit err
			}
		}else{
			n_btab++;
			btab[n_btab] = $0
		}
	}
}
{
	if(NR == 1){
		phdr = $0
		p_mkey = -1 
		for(i = 1; i <= NF; i++){
			if($i == mkey){
				p_mkey = i
				break
			}
		}
		if(p_mkey == -1){
			printf("ERROR: main: mkey \"%s\" is not in hdr of pfile \"%s\"\n", mkey, FILENAME == "-" ? "__stdin__" : FILENAME) > "/dev/stderr"
			err = 1
			exit err
		}
	}else{
		n_ptab++
		ptab[$p_mkey] = $0
		pused[$p_mkey] = 0
	}
}
END {
	if(err)
		exit err

	# print the combined header
	printf("%s", bhdr)
	n_ary = split(phdr, ary, "\t")
	for(i = 1; i < p_mkey; i++)
		printf("\t%s", ary[i])
	for(i = p_mkey + 1; i <= n_ary; i++)
		printf("\t%s", ary[i])
	printf("\n")

	# add the new columns based on mkey
	for(i = 1; i <= n_btab; i++){
		n_ary = split(btab[i], ary, "\t")
		p_val = ary[b_mkey]
		if(!(p_val in ptab)){
			printf("WARN: END: no properties for key \"%s\" in ptab\n", p_val) > "/dev/stderr"
			err = 1
			continue
		}
		pused[p_val] = 1
		printf("%s", btab[i])
		n_ary = split(ptab[p_val], ary, "\t")
		for(j = 1; j < p_mkey; j++)
			printf("\t%s", ary[j])
		for(j = p_mkey + 1; j <= n_ary; j++)
			printf("\t%s", ary[j])
		printf("\n")
	}

	# check if all the new rows were used
	for(k in ptab){
		if(!pused[k]){
			printf("WARN: END: new values for key \"%s\" were not used\n", k) > "/dev/stderr"
			err = 1
			continue
		}
	}

}' $FILE
