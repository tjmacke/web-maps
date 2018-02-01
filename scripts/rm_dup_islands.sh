#! /bin/bash
#
FILE=$1

awk -F'\t' '{
	lines[$2] = $0
}
END {
	for(k in lines)
		print lines[k]
}' $FILE
