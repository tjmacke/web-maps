#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage; $0 [ -help ] -pk primary-key [ props-tsv-file ]"

PKEY=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-pk)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-pk requires primary-key argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		PKEY=$1
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

if [ -z "$PKEY" ] ; then
	LOG ERROR "missing -pk primary-key argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

awk -F'\t' 'BEGIN {
	pkey = "'"$PKEY"'"
}
NR == 1 {
	pk_fnum = 0
	ti_fnum = 0
	n_htab = 0
	for(i = 1; i <= NF; i++){
		if($i == pkey)
			pk_fnum = i
		if($i == "title"){
			if(ti_fnum == 0)
				ti_fnum = i
			else{
				printf("ERROR: main: field named \"title\" appears more than once in the header\n") > "/dev/stderr"
				err = 1
				exit err
			}
		}
		n_htab++
		htab[i] = $i
	}
	if(!pk_fnum){
		printf("ERROR: main: primary key %s is not in list of fields\n", pkey) > "/dev/stderr"
		err = 1
		exit err
	}
	if(!ti_fnum){
		printf("ERROR: main: no field is named \"title\"\n") > "/dev/stderr"
		err = 1
		exit err
	}
	next
}
{
	printf("%d\t{", $pk_fnum)
	for(i = 1; i <= NF; i++){
		printf("\"%s\": %s%s", htab[i], mk_json_value($i), i < NF ? ", " : "")
	}
	printf("}\n")
}
END {
	if(err)
		exit err
}
function mk_json_value(str,   work, l_pfx) {

	work = str
	if(substr(work, 1, 1) == "-")
		work = substr(work, 2)
	# ints
	if(work == "0")
		return str
	if(work ~ /^[1-9][0-9]*$/)
		return str

	# floats.  NB: digits requireed after .; only one 0 alloed before . or E
	l_pfx = 0
	if(match(work, /^0\.[0-9][0-9*]/))
		l_pfx = RLENGTH
	else if(match(work, /^[1-9][0-9]*\.[0-9][0-9]*/))
		l_pfx = RLENGTH
	else if(match(work, /^0[Ee]/))
		l_pfx = RLENGTH - 1
	else if(match(work, /^[1-9][0-9]*[Ee]/))
		l_pfx = RLENGTH - 1

	if(l_pfx > 0){
		work = substr(work, l_pfx + 1)
		if(work == "")
			return str
		if(work ~ /^[Ee]/){
			work = substr(work, 2)
			if(work ~ /^[+-]/)
				work = substr(work, 2)
			if(work ~ /^[0-9][0-9]*$/)
				return str
		}
	}

	work = str
	gsub(/"/, "\\\"", work)

	
	return "\"" work "\""
}' $FILE
