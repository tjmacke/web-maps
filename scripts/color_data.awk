BEGIN {
	# I got the palettes from www.color-hex.com/color-palette/

	# Princess Pink, 6653
	all_colors["pink",        1] = "#ffc2cd"
	all_colors["pink",        2] = "#ff93ac"
	all_colors["pink",        3] = "#ff6290"
	all_colors["pink",        4] = "#ff3468"
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

	# No bcolor: Uses [(pink, gold, green, blue, purple), 3]
	all_colors["_NO_BCOLOR_", 1] = all_colors["pink",   3]
	all_colors["_NO_BCOLOR_", 2] = all_colors["gold",   3]
	all_colors["_NO_BCOLOR_", 3] = all_colors["green",  3]
	all_colors["_NO_BCOLOR_", 4] = all_colors["blue",   3]
	all_colors["_NO_BCOLOR_", 5] = all_colors["purple", 3]

	# assign names to the _NO_BCOLOR_ values
	cval_to_cname[all_colors["pink",   3]] = "pink"
	cval_to_cname[all_colors["gold",   3]] = "gold"
	cval_to_cname[all_colors["green",  3]] = "green"
	cval_to_cname[all_colors["blue",   3]] = "blue"
	cval_to_cname[all_colors["purple", 3]] = "purple"
}
