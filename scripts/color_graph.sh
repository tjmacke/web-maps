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
awk -F'\t' 'BEGIN {
	verbose = "'"$VERBOSE"'" == "yes"
	bcolor = ("'"$BCOLOR"'" != "") ? "'"$BCOLOR"'" : "_NO_BCOLOR_"
	id = "'"$ID"'"

	## BEGIN color defs
	# I got these palettes from www.color-hex.com/color-palette/

	# No bcolor [(uses pink, gold, green, blue, purple), 3]
	all_colors["_NO_BCOLOR_", 1] = "#ff6290"
	all_colors["_NO_BCOLOR_", 2] = "#ffbf00"
	all_colors["_NO_BCOLOR_", 3] = "#028900"
	all_colors["_NO_BCOLOR_", 4] = "#7ba4dd"
	all_colors["_NO_BCOLOR_", 5] = "#f3b4ff"

	# Princess Pink, 6653
	all_colors["pink",        1] = "#ffc2cd"
	all_colors["pink",        2] = "#ff93ac"
	all_colors["pink",        3] = "#ff6289"
	all_colors["pink",        4] = "#fc3468"
	all_colors["pink",        5] = "#ff084a"
	
	# 24K GOLD, 2799
	all_colors["gold",        1] = "#a67c00"
	all_colors["gold",        2] = "#bf9b30"
	all_colors["gold",        3] = "#ffbf00"
	all_colors["gold",        4] = "#ffcf40"
	all_colors["gold",        5] = "#ffdc73"

	# I Loved in Shades of Green, 1325
	all_colors["green",       1] = "#adff00"
	all_colors["green",       2] = "#74d600"
	all_colors["green",       3] = "#028900"
	all_colors["green",       4] = "#00d27f"
	all_colors["green",       5] = "#00ff83"

	# 5 shades of blue, 44020
	all_colors["blue",        1] = "#abbcea"
	all_colors["blue",        2] = "#92b4e3"
	all_colors["blue",        3] = "#7ba4dd"
	all_colors["blue",        4] = "#628dd6"
	all_colors["blue",        5] = "#4c74c9"

	# shades of purple rain, 45572
	all_colors["purple",      1] = "#fce4ff"
	all_colors["purple",      2] = "#fad2ff"
	all_colors["purple",      3] = "#f3b4ff"
	all_colors["purple",      4] = "#ed8fff"
	all_colors["purple",      5] = "#e24cff"

	# assign names to the _NO_BCOLOR_ values
	cval_to_cname["#ff6290"] = "pink"
	cval_to_cname["#ffbf00"] = "gold"
	cval_to_cname["#028900"] = "green"
	cval_to_cname["#7ba4dd"] = "blue"
	cval_to_cname["#f3b4ff"] = "purple"

	#
	## END color defs

	colors["n_colors"] = 5

	colors["colors", 1] = c1 = all_colors[bcolor, 1]
	colors["colors", 2] = c2 = all_colors[bcolor, 2]
	colors["colors", 3] = c3 = all_colors[bcolor, 3]
	colors["colors", 4] = c4 = all_colors[bcolor, 4]
	colors["colors", 5] = c5 = all_colors[bcolor, 5]
	colors["colors", c1] = 1
	colors["colors", c2] = 2
	colors["colors", c3] = 3
	colors["colors", c4] = 4
	colors["colors", c5] = 5

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
