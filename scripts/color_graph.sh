#! /bin/bash
#
. ~/etc/funcs.sh

# input:
#	node-num	node-name	edge-count	edge-list

FILE=$1

sort -t $'\t' -k 3rn,3 $FILE	|\
awk -F'\t' '{
	n_nodes++
	graf[n_nodes, "num"] = $1
	graf[n_nodes, "name"] = $2
	graf[n_nodes, "c_neighbors"] = $3
	graf[n_nodes, "neighbors"] = $4
	name_to_node[$2] = n_nodes
	stacked[n_nodes] = 0
}
END {
	for(done = 0; !done; ){
		n_rm = 0
		for(i = 1; i <= n_nodes; i++){
			if(graf[i, "c_neighbors"] == 0)
				n_rm++
			else if(graf[i, "c_neighbors"] <= 4)
				remove_node(graf, i, name_to_node)
		}
		dump_graf("/dev/stderr", n_nodes, graf)
		done = n_rm == n_nodes
	}
}
function remove_node(graf, n, name_to_node,    nf, ary, i) {
	nf = split(graf[n, "neighbors"], ary, "|")
	for(i = 1; i <= nf; i++){
		ni = name_to_node[ary[i]]
		if(graf[ni, "c_neighbors"] > 0){
			graf[ni, "c_neighbors"]--
			if(graf[ni, "c_neighbors"] == 0)
				printf("%d\t%s\t%s\n", ni, graf[ni, "name"], graf[ni, "neighbors"])
		}
	}
	graf[n, "c_neighbors"] = 0
	printf("%d\t%s\t%s\n", n, graf[n, "name"], graf[n, "neighbors"])
}
function dump_graf(file, n_nodes, graf,   i) {

	for(i = 1; i <= n_nodes; i++)
		printf("%d\t%s\t%d\t%s\n", graf[i, "num"], graf[i, "name"], graf[i, "c_neighbors"], graf[i, "neighbors"]) > file
}'
