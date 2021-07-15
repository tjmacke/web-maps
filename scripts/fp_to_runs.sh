#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ fp-mon-dir ]"

DIR=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		DIR=$1
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

if [ -z "$DIR" ] ; then
	LOG ERROR "missing fp-mon-dir argumnent"
	echo "$U_MSG" 1>&2
	exit 1
fi

SD_FILE=$DIR/shift.data
if [ ! -s $SD_FILE ] ; then
	LOG ERROR "shift data file $SD_FILE either doesn't exist or is empty"
	exit 1
fi

cat $DIR/*.log	|
awk 'BEGIN {
	sd_file = "'"$SD_FILE"'"
	n_sdlines = n_sdtab = 0
	while((getline sd_line < sd_file) > 0){
		n_sdlines++
		if(n_sdlines > 1){
			n_ary = split(sd_line, ary, "\t")
			n_sdtab++
			sdtab[ary[1] "_" ary[2]] = n_sdtab
			tStart[n_sdtab] = ary[3]
			tEnd[n_sdtab] = ary[4]
			mStart[n_sdtab] = ary[5]
			mEnd[n_sdtab] = ary[6]
			nJobs[n_sdtab] = ary[7]
			pay[n_sdtab] = ary[8]
		}
	}
}
{
	n_lines++
	lines[n_lines] = $0
}
END {
	l_date = ""
	l_app = ""
	for(i = 1; i <= n_lines; i++){
		parse_aline(lines[i], ainfo)
		da_start = set_da_start(lines, i, ainfo)
		if(l_date == ""){
			printf("Date\ttStart\ttEnd\tMileage\tjobType\tlocStart\tlocEnd\tAmount\tPayment\tNotes\n")
			sd_idx = sdtab[ainfo["date"] "_" ainfo["app"]]
			printf("%s\t%s\t.\t.\tBEGIN\n", ainfo["date"], tStart[sd_idx])
		}else if(ainfo["date"] != l_date){
			printf("%s\t.\t%s\t.\tEND\n", l_date, tEnd[sd_idx])
			sd_idx = sdtab[ainfo["date"] "_" ainfo["app"]]
			printf("%s\t%s\t.\t.\tBEGIN\n", ainfo["date"], tStart[sd_idx])
		}else if(ainfo["app"] != l_app){
			# app starting w/+ indicates >1 app running
			if(substr(ainfo["app"], 1, 1) != "+"){
				printf("%s\t.\t%s\t.\tEND\n", l_date, tEnd[sd_idx])
				sd_idx = sdtab[ainfo["date"] "_" ainfo["app"]]
				printf("%s\t%s\t.\t.\tBEGIN\n", ainfo["date"], tStart[sd_idx])
			}
		}
			
		for(d = da_start; d <= ainfo["n_atab"]; d++){
			printf("%s\t%s", ainfo["date"], ainfo["time"])
			printf("\t.\t.\tJob")
			printf("\t%s\t%s", ainfo["atab", 1], ainfo["atab", d])

			# Payment was DD only, use -1 to indicate N/A
			# The Payment field was used to indicate payment via DD red card.
			# Then added caviar and grubhub, neither of which had payment cards, so -1 means no longer used.
			# And then grubhub added a payment card, but too bad.  Never used the info anyway
			printf("\t-1")

			# Never expected to run > 1 app at the same time. Did. Once. Bad idea. Nveer again.
			# Use +app to indicate 2d app.  Drop the + before printing
			printf("\t%s\t.", substr(ainfo["app"], 1, 1) == "+" ? substr(ainfo["app"], 2) : ainfo["app"])

			printf("\n")
		}
		l_date = ainfo["date"]
		# do not update l_app if app begins with + as the "dash" info should not change.
		if(substr(ainfo["app"], 1, 1) != "+")
			l_app = ainfo["app"]
	}
	if(n_lines > 0)
		printf("%s\t.\t%s\t.\tEND\n", l_date, tEnd[sd_idx])
}
function parse_aline(line, ainfo,   n_ary, ary, n_dt, dt, i) {
	n_ary = split(line, ary)
	n_dt = split(ary[1], dt, "_")
	ainfo["date"] = sprintf("%s-%s-%s", substr(dt[1], 1, 4), substr(dt[1], 5, 2), substr(dt[1], 7, 2))
	ainfo["time"] = sprintf("%s:%s", substr(dt[2], 1, 2), substr(dt[2], 3, 2))
	ainfo["app"] = "gh"
	ainfo["alist"] = ""
	for(i = 2; i <= n_ary; i++){
		if(ary[i] == "-app"){
			i++
			ainfo["app"] = ary[i]
		}else if(ary[i] == "-a"){
			i++
			ainfo["alist"] = ary[i]
		}else if(ainfo["alist"]){
			ainfo["alist"] = ainfo["alist"] " " ary[i]
		}
	}
	n_ary = split(ainfo["alist"], ary, "|")
	ainfo["n_atab"] = n_ary
	for(i = 1; i <= n_ary; i++){
		if(i == 1)
			sub(/^rest:  */, "", ary[i])
		else
			sub(/^  *[1-9]\.  */, "", ary[i])
		sub(/  *$/, "", ary[i])
		ainfo["atab", i] = ary[i]
	}
}
# Sometimes additional orders are added to an active order
# This results in the case where the 1st order line is
#
# ... src | dst
#
# and subsequent lines add | dst to this pfx.
#
# ... src | dst | dst ...
#
# So if the previous line is is a pfx of the current line,
# start at the last dst as previous dst have already been handled
function set_da_start(lines, ln, ainfo,   p_ainfo) {
	if(ln == 1)
		return 2
	else if(ainfo["n_atab"] == 2)
		return 2
	else{
		parse_aline(lines[ln-1], p_ainfo)
		if(index(ainfo["alist"], p_ainfo["alist"]) == 1)
			return p_ainfo["n_atab"] + 1
		else
			return 2
	}
}'
