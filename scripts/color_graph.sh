#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -trace ] [ -v ] [ -bc { pink | gold | green | blue | purple } ] -id id-field [ ap-file ]"

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi
WM_SCRIPTS=$WM_HOME/scripts

# awk v3 does not support includes
AWK_VERSION="$(awk --version | awk '{ nf = split($3, ary, /[,.]/) ; print ary[1] ; exit 0 }')"
if [ "$AWK_VERSION" == "3" ] ; then
	AWK="igawk --re-interval"
	COLOR_DATA="$WM_SCRIPTS/color_data.awk"
elif [ "$AWK_VERSION" == "4" ] ; then
	AWK=awk
	COLOR_DATA="\"$WM_SCRIPTS/color_data.awk\""
else
	LOG ERROR "unsupported awk version: \"$AWK_VERSION\": must be 3 or 4"
	exit 1
fi

# adjacency file format:
#	node-num	node-name	edge-count	edge-list
#
# edge-list is a pipe (|) separated list of node-names.  node-num is carried along and may be used by other programs

TRACE=
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
	-trace)
		TRACE="yes"
		shift
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
$AWK -F'\t' '
@include '"$COLOR_DATA"'
BEGIN {
	trace = "'"$TRACE"'" == "yes"
	if(trace)
		trace_f = 1
	verbose = "'"$VERBOSE"'" == "yes"
	bcolor = ("'"$BCOLOR"'" != "") ? "'"$BCOLOR"'" : "_NO_BCOLOR_"
	id = "'"$ID"'"

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
	if(trace_f){
		trace_f = 0
		printf("TRACE: BEGIN: color graph\n") > "/dev/stderr"
	}
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
	if(err)
		exit err

	printf("%s\t%s\n", id, "fill")
	if(n_nodes > 0){
		node_stk["stkp"] = 0
		for(done = 0; !done; ){
			n_rm = 0
			need_r2 = 1
			for(i = 1; i <= n_nodes; i++){
				if(graf[i, "c_neighbors"] == 0){
					n_rm++
					need_r2 = 0
				}else if(graf[i, "c_neighbors"] <= 4){
					remove_node(graf, i, name_index, node_stk)
					need_r2 = 0
				}
			}
			if(need_r2){
				err = 1
				printf("ERROR: need rule 2!\n") > "/dev/stderr"
				dump_graf("/dev/stderr", n_nodes, graf)
				if(trace)
					printf("TRACE: END: FAILED: color graph needs rule 2\n") > "/dev/stderr"
				exit err
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
#		dump_graf("/dev/stdout", n_nodes, graf)
		for(i = 1; i <= n_nodes; i++)
			printf("%s\t%s\n", graf[i, "name"], graf[i, "color"])
	}

	# color the islands
	for(i = 1; i <= colors["n_colors"]; i++)
		colors["used", i] = 0
	for(i = 1; i <= n_islands; i++){
		printf("%s\t%s\n", islands[i, "name"], get_color(colors));
	}

	if(trace)
		printf("TRACE: END: color graph\n") > "/dev/stderr"
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
