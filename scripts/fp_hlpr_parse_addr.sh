#! /bin/bash
#
echo "$1" |
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
		sub(/^  */, "", pfx)
		sub(/  *$/, "", pfx)
		sub(/^  */, "", addr)
		sub(/  *$/, "", addr)
		printf("%s\t%s\n", pfx, addr)
	}
}'
