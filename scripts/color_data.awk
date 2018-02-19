BEGIN {
	# pink
	all_colors["pink", 1] = "#ffa9bd"
	all_colors["pink", 2] = "#ff98b0"
	all_colors["pink", 3] = "#ff86a4"
	all_colors["pink", 4] = "#ff7598"
	all_colors["pink", 5] = "#ff638b"

	# gold
	all_colors["gold", 1] = "#c39a17"
	all_colors["gold", 2] = "#cda723"
	all_colors["gold", 3] = "#d7b32f"
	all_colors["gold", 4] = "#e1bf3b"
	all_colors["gold", 5] = "#ebcb46"

	# green
	all_colors["green", 1] = "#96d600"
	all_colors["green", 2] = "#70d620"
	all_colors["green", 3] = "#4bd640"
	all_colors["green", 4] = "#25d561"
	all_colors["green", 5] = "#00d581"

	# blue
	all_colors["blue", 1] = "#92b0e3"
	all_colors["blue", 2] = "#80a2dd"
	all_colors["blue", 3] = "#6e94d7"
	all_colors["blue", 4] = "#5d86d1"
	all_colors["blue", 5] = "#4b79cb"

	# purple
	all_colors["purple", 1] = "#f4b9ff"
	all_colors["purple", 2] = "#f2acff"
	all_colors["purple", 3] = "#ef9eff"
	all_colors["purple", 4] = "#ed90ff"
	all_colors["purple", 5] = "#eb82ff"

	# No bcolor: Uses [(pink, gold, green, blue, purple), 3]
	all_colors["_NO_BCOLOR_", 1] = all_colors["pink", 3]
	all_colors["_NO_BCOLOR_", 2] = all_colors["gold", 3]
	all_colors["_NO_BCOLOR_", 3] = all_colors["green", 3]
	all_colors["_NO_BCOLOR_", 4] = all_colors["blue", 3]
	all_colors["_NO_BCOLOR_", 5] = all_colors["purple", 3]

	# Assign names to the _NO_COLOR_ values
	cval_to_cname[all_colors["pink", 3]] = "pink"
	cval_to_cname[all_colors["gold", 3]] = "gold"
	cval_to_cname[all_colors["green", 3]] = "green"
	cval_to_cname[all_colors["blue", 3]] = "blue"
	cval_to_cname[all_colors["purple", 3]] = "purple"
}
