function WM_min(a, b) {
	return a < b ? a : b
}
function WM_max(a, b) {
	return a > b ? a : b
}
function WM_crosses(x, y, iv, m, b, x1, ymin,   ans) {

	if(iv)
		ans =  x < x1
	else if(m != 0){
		ans = y != ymin && x < (y - b)/m
	}else
		ans = 0
	return ans
}
function WM_read_polys(lfile, is_chull, iv_tab, m_tab, b_tab, x1_tab, xmin_tab, ymin_tab, xmax_tab, ymax_tab, l_first, l_cnt, chull,  name, n_poly, cnt, n_lines, x1, y1, x2, y2) {

	name = ""
	n_poly = 0
	cnt = 0;
	for(n_lines = 0; (getline < lfile) > 0; ){
		n_lines++
		iv_tab[n_lines] = $3 + 0
		m_tab[n_lines] = $4 + 0
		b_tab[n_lines] = $5 + 0
		x1 = $6 + 0
		y1 = $7 + 0
		x2 = $8 + 0
		y2 = $9 + 0
		x1_tab[n_lines] = x1
		y1_tab[n_lines] = y1	# used only for -is_chull
		# create the bounding box
		xmin_tab[n_lines] = WM_min(x1, x2)	# not used, delete?
		ymin_tab[n_lines] = WM_min(y1, y2)
		xmax_tab[n_lines] = WM_max(x1, x2)
		ymax_tab[n_lines] = WM_max(y1, y2)
		if($1 != name){
			n_poly++
			if(name != "")
				l_cnt[name] = cnt
			cnt = 0
			l_first[$1] = n_lines;
		}
		name = $1
		cnt++
	}
	l_cnt[name] = cnt
	close(lfile)

	if(is_chull){
		if(n_poly > 1)
			return sprintf("ERROR: BEGIN: too many polygons: %d : is_chull requires only 1", n_poly)
		for(i = 1; i < n_lines; i++)
			chull[x1_tab[i], y1_tab[i]] = 1
	}

#	for(nb in l_first)
#		printf("%d\t%d\t%s\n", l_first[nb], l_cnt[nb], nb) > "/dev/stderr"

	return ""
}
function WM_put_point_in_poly(is_chull, n_pcands, pcands, x, y, iv_tab, m_tab, b_tab, x1_tab, xmin_tab, ymin_tab, xmax_tab, ymax_tab, l_first, l_cnt, chull,    pc, nb, nc, lf, l, ix) {

	for(pc = 1; pc <= n_pcands; pc++){
		nb = pcands[pc]
		nc = 0
		lf = l_first[nb]
		for(l = 0; l < l_cnt[nb]; l++){
			ix = lf + l
			if(y > ymax_tab[ix])
				continue;	# above
			else if(y < ymin_tab[ix])
				continue	# below
			else if(x > xmax_tab[ix])
				continue	# to the right
			nc += WM_crosses(x, y, iv_tab[ix], m_tab[ix], b_tab[ix], x1_tab[ix], ymin_tab[ix])
		}
		# odd number of crosses means in the polygon
		if(nc % 2){
			printf("%s\t%s\n", $2, nb)
			break
		}
		# if the only poly is a convex hull, then points on the
		# N & E parts of the hull are in the hull even though they
		# do not cross it
		if(is_chull){
			if(chull[x,y]){
				printf("%s\t%s\n", $2, nb)
				break
			}
		}
	}
}
