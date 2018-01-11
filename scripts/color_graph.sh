#! /bin/bash
#
. ~/etc/funcs.sh

# input:
#	node-num	node-name	edge-count	edge-list

FILE=$1

sort -t $'\t' -k 3rn,3 $FILE	|\
awk -F'\t' ' BEGIN {
	verbose = 0
	colors["n_colors"] = 5
	colors["colors", 1] = "red"
	colors["colors", 2] = "orange"
	colors["colors", 3] = "yellow"
	colors["colors", 4] = "green"
	colors["colors", 5] = "cyan"
	colors["colors", "red"] = 1
	colors["colors", "orange"] = 2
	colors["colors", "yellow"] = 3
	colors["colors", "green"] = 4
	colors["colors", "cyan"] = 5
	colors["used", 1] = 0
	colors["used", 2] = 0
	colors["used", 3] = 0
	colors["used", 4] = 0
	colors["used", 5] = 0
	colors["next"] = 1
}
{
	n_nodes++
	graf[n_nodes, "num"] = $1
	graf[n_nodes, "name"] = $2
	graf[n_nodes, "c_neighbors"] = $3
	graf[n_nodes, "neighbors"] = $4
	graf[n_nodes, "color"] = ""
	name_index[$2] = n_nodes
	stacked[n_nodes] = 0
}
END {
	node_stk["stkp"] = 0
	for(done = 0; !done; ){
		n_rm = 0
		for(i = 1; i <= n_nodes; i++){
			if(graf[i, "c_neighbors"] == 0)
				n_rm++
			else if(graf[i, "c_neighbors"] <= 4)
				remove_node(graf, i, name_index, node_stk)
		}
		if(verbose)
			dump_graf("/dev/stderr", n_nodes, graf)
		done = n_rm == n_nodes
	}
	for(i = node_stk["stkp"]; i >= 1; i--){
#		printf("%s\n", node_stk["stk", i])
		color_node(graf, node_stk["stk", i], name_index, colors)
	}
	dump_graf("/dev/stdout", n_nodes, graf)
}
function remove_node(graf, n, name_index, node_stk,    nf, ary, i, ni) {
	nf = split(graf[n, "neighbors"], ary, "|")
	for(i = 1; i <= nf; i++){
		ni = name_index[ary[i]]
		if(graf[ni, "c_neighbors"] > 0){
			graf[ni, "c_neighbors"]--
			if(graf[ni, "c_neighbors"] == 0){
#				printf("%d\t%s\t%s\n", ni, graf[ni, "name"], graf[ni, "neighbors"])
				node_stk["stkp"]++
				node_stk["stk", node_stk["stkp"]] = graf[ni, "name"]
			}
		}
	}
	graf[n, "c_neighbors"] = 0
#	printf("%d\t%s\t%s\n", n, graf[n, "name"], graf[n, "neighbors"])
	node_stk["stkp"]++
	node_stk["stk", node_stk["stkp"]] = graf[n, "name"]
}
function color_node(graf, name, name_index, colors,    nf, ary, i, n, ni, c) {

	for(i = 1; i <= colors["n_colors"]; i++)
		colors["used", i] = 0
#	printf("DEBUG: color_node: color %s\n", name) > "/dev/stderr"
	n = name_index[name]
	nf = split(graf[n, "neighbors"], ary, "|")
	for(i = 1; i <= nf; i++){
		ni = name_index[ary[i]]
		if(graf[ni, "color"] != "")
			colors["used", colors["colors", graf[ni, "color"]]] = 1
#		printf("DEBUG: color_node: chk neighbor %s, color = %s\n", ary[i], graf[ni, "color"] == "" ? "_notSet_" : graf[ni, "color"]) > "/dev/stderr"
	}
	c = get_color(colors)
	if(c == ""){
		printf("ERROR: color_node: get_color failed for node %s\n", name) > "/dev/stderr"
		exit 1
	}
	graf[n, "color"] = c
#	printf("\n") > "/dev/stderr"
}
function get_color(colors,   i, c) {

	c = ""
	for(i = 1; i <= colors["n_colors"]; i++){
		if(colors["used", colors["next"]] == 0)
			c = colors["colors", colors["next"]]
		colors["next"] = (colors["next"] + 1) % colors["n_colors"] == 0 ? colors["n_colors"] : (colors["next"] + 1) % 5
		if(c != "")
			break
	}
	return c
}
function dump_graf(file, n_nodes, graf,   i) {

	for(i = 1; i <= n_nodes; i++)
		printf("%d\t%s\t%s\t%d\t%s\n", graf[i, "num"], graf[i, "name"], graf[i, "color"] == "" ? "_notSet" : graf[i, "color"], graf[i, "c_neighbors"], graf[i, "neighbors"]) > file
}'
