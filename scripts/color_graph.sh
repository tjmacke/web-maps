#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -v ] [ -bc { pink | gold | green | blue | purple } ] -id id-field [ ap-file ]"

# ap-file format:
#	node-num	node-name	edge-count	edge-list

VERBOSE=
BCOLOR=
ID=
FILE=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-v)
		VERBOSE="yes"
		shift
		;;
	-bc)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-bc requires base-color argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		BCOLOR=$1
		shift
		;;
	-id)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-id requires id-field argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		ID="$1"
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

if [ ! -z "$BCOLOR" ] ; then
	if [ "$BCOLOR" != "pink" ] && [ "$BCOLOR" != "gold" ] && [ "$BCOLOR" != "green" ] && [ "$BCOLOR" != "blue" ] && [ "$BCOLOR" != "purple" ] ; then
		LOG ERROR "bad base color $BCOLOR, must be pink, gold, green, blue or purple"
		echo "$U_MSG" 1>&2
		exit 1
	fi
fi

if [ -z "ID" ] ; then
	LOG ERROR "missing -id -id-field argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

sort -t $'\t' -k 3rn,3 $FILE	|\
awk -F'\t' ' BEGIN {
	verbose = "'"$VERBOSE"'" == "yes"
	bcolor = "'"$BCOLOR"'"
	id = "'"$ID"'"
	pink = "#ff6290"
	gold = "#ffbf00"
	green = "#028900"
	blue = "#7ba4dd"
	purple = "#f3b4ff"
	colors["n_colors"] = 5
	# TODO: if bcolor != "", then 1,2,3,4,5 begome bcolor, bcolor+1, bcolor+2, bcolor-2, bcolor-1
	colors["colors", 1] = pink
	colors["colors", 2] = gold
	colors["colors", 3] = green
	colors["colors", 4] = blue
	colors["colors", 5] = purple
	colors["colors", pink] = 1
	colors["colors", gold] = 2
	colors["colors", green] = 3
	colors["colors", blue] = 4
	colors["colors", purple] = 5
	colors["used", 1] = 0
	colors["used", 2] = 0
	colors["used", 3] = 0
	colors["used", 4] = 0
	colors["used", 5] = 0
	colors["next"] = 1
}
{
	# deal with "islands" separately
	if($3 == 0){
		n_islands++
		islands[n_islands, "num"] = $1
		islands[n_islands, "name"] = $2
		islands[n_islands, "c_neighbors"] = $3
		islands[n_islands, "neighbors"] = $4
	}else{
		n_nodes++
		graf[n_nodes, "num"] = $1
		graf[n_nodes, "name"] = $2
		graf[n_nodes, "c_neighbors"] = $3
		graf[n_nodes, "neighbors"] = $4
		graf[n_nodes, "color"] = ""
		name_index[$2] = n_nodes
		stacked[n_nodes] = 0
	}
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
		if(verbose)
			printf("DEBUG: END: %s\n", node_stk["stk", i]) > "/dev/stderr"
		color_node(graf, node_stk["stk", i], name_index, colors)
	}
#	dump_graf("/dev/stdout", n_nodes, graf)
	printf("%s\t%s\n", id, "fill")
	for(i = 1; i <= n_nodes; i++)
		printf("%s\t%s\n", graf[i, "name"], graf[i, "color"])

	# color the islands
	for(i = 1; i <= colors["n_colors"]; i++)
		colors["used", i] = 0
	for(i = 1; i <= n_islands; i++){
		printf("%s\t%s\n", islands[i, "name"], get_color(colors));
	}
}
function remove_node(graf, n, name_index, node_stk,    nf, ary, i, ni) {
	nf = split(graf[n, "neighbors"], ary, "|")
	for(i = 1; i <= nf; i++){
		ni = name_index[ary[i]]
		if(graf[ni, "c_neighbors"] > 0){
			graf[ni, "c_neighbors"]--
			if(graf[ni, "c_neighbors"] == 0){
				if(verbose)
					printf("DEBUG: remove_node: %d\t%s\t%s\n", ni, graf[ni, "name"], graf[ni, "neighbors"]) > "/dev/stderr"
				node_stk["stkp"]++
				node_stk["stk", node_stk["stkp"]] = graf[ni, "name"]
			}
		}
	}
	graf[n, "c_neighbors"] = 0
	if(verbose)
		printf("DEBUG: remove_node: %d\t%s\t%s\n", n, graf[n, "name"], graf[n, "neighbors"]) > "/dev/stderr"
	node_stk["stkp"]++
	node_stk["stk", node_stk["stkp"]] = graf[n, "name"]
}
function color_node(graf, name, name_index, colors,    nf, ary, i, n, ni, c) {

	for(i = 1; i <= colors["n_colors"]; i++)
		colors["used", i] = 0
	if(verbose)
		printf("DEBUG: color_node: color %s\n", name) > "/dev/stderr"
	n = name_index[name]
	nf = split(graf[n, "neighbors"], ary, "|")
	for(i = 1; i <= nf; i++){
		ni = name_index[ary[i]]
		if(graf[ni, "color"] != "")
			colors["used", colors["colors", graf[ni, "color"]]] = 1
		if(verbose)
			printf("DEBUG: color_node: chk neighbor %s, color = %s\n", ary[i], graf[ni, "color"] == "" ? "_notSet_" : graf[ni, "color"]) > "/dev/stderr"
	}
	c = get_color(colors)
	if(c == ""){
		printf("ERROR: color_node: get_color failed for node %s\n", name) > "/dev/stderr"
		exit 1
	}
	graf[n, "color"] = c
	if(verbose)
		printf("\n") > "/dev/stderr"
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
