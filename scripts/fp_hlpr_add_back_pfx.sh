#! /bin/bash
#
echo "$1" | awk -F'|' '{
	if($1 == "")
		print $2
	else{
		n_ary = split($2, ary, "\t")
		ary[2] = $1 ary[2]
		printf("%s", ary[1])
		for(i = 2; i <= n_ary; i++)
			printf("\t%s", ary[i])
		printf("\n")
	}
}'
