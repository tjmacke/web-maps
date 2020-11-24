#! /bin/bash
#
. ~/etc/funcs.sh
export LC_ALL=C

U_MSG="usage: $0 [ -help ] -fmt { cfg | html } -sname N [ -grad ] -h N[:N] [ -s N[:N] ] [ -v N[:N] ] n_colors"

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME is not defined"
	exit 1
fi
DM_LIB=$DM_HOME/lib

# awk v3 does not support include
AWK_VERSION="$(awk --version | awk '{ nf = split($3, ary, /[,.]/) ; print ary[1] ; exit 0 }')"
if [ "$AWK_VERSION" == "3" ] ; then
	AWK="igawk --re-interval"
	COLOR_UTILS="$DM_LIB/color_utils.awk"
elif [ "$AWK_VERSION" == "4" ] ; then
	AWK=awk
	COLOR_UTILS="\"$DM_LIB/color_utils.awk\""
else
	LOG ERROR "unsupported awk version: \"$AWK_VERSION\": must be 3 or 4"
	exit 1
fi

FMT=
SNAME=
GRAD=
H=
S="0.7"
V=1
N_COLORS=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG" 1>&2
		exit 0
		;;
	-fmt)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-fmt requires format argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		FMT=$1
		shift
		;;
	-sname)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-sname reqquires scale name argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		SNAME="$1"
		shift
		;;
	-grad)
		GRAD="yes"
		shift
		;;
	-h)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-h requires hue argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		H=$1
		shift
		;;
	-s)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-s requires saturation argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		S="$1"
		shift
		;;
	-v)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-v requires value argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		V="$1"
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		N_COLORS=$1
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

if [ -z "$FMT" ] ; then
	LOG ERROR "missing -fmt { cfg | html } argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ "$FMT" == "cfg" ] ; then
	if [ -z "$SNAME" ] ; then
		LOG ERROR "-fmt cnt rquires -sname name"
		echo "$U_MSG" 1>&2
		exit
	fi
elif [ "$FMT" != "html" ] ; then
	LOG ERROR "unknown format \"$FMT\" must be cfg or html"
fi

if [ -z "$H" ] ; then
	LOG ERROR "missing -h hue argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$N_COLORS" ] ; then
	LOG ERROR "missing n_colors argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ $N_COLORS -lt 1 ] ; then
	LOG ERROR "bad n_colors $N_COLORS, must be >= 1"
	exit 1
fi

$AWK '
@include '"$COLOR_UTILS"'
BEGIN {
	fmt = "'"$FMT"'"
	sname = "'"$SNAME"'"
	grad = "'"$GRAD"'" == "yes"

	n_ary = split("'"$H"'", ary, ":")
	h_start = h_end = ary[1] + 0
	if(n_ary > 1)
		h_end = ary[1] + 0
	if(h_start < 0 || h_start > 360){
		printf("ERROR: BEGIN: bad h_start: %5.3f, must be in [0,360]\n", h_start) > "/dev/stderr"
		err = 1
		exit err
	}
	if(h_end < 0 || h_end > 360){
		printf("ERROR: BEGIN: bad h_end: %5.3f, must be in [0,360]\n", h_end) > "/dev/stderr"
		err = 1
		exit err
	}

	n_ary = split("'"$S"'", ary, ":")
	s_start = s_end = ary[1] + 0
	if(n_ary > 1)
		s_end = ary[2] + 0
	s = "'"$S"'"
	if(s_start < 0 || s_start > 1){
		printf("ERROR: BEGIN: bad s_start: %5.3f, must be in [0,1]\n", s_start) > "/dev/stderr"
		err = 1
		exit err
	}
	if(s_end < 0 || s_end > 1){
		printf("ERROR: BEGIN: bad s_end: %5.3f, must be in [0,1]\n", s_end) > "/dev/stderr"
		err = 1
		exit err
	}

	n_ary = split("'"$V"'", ary, ":")
	v_start = v_end = ary[1] + 0
	if(n_ary > 1)
		v_end = ary[2] + 0
	if(v_start < 0 || v_start > 1){
		printf("ERROR: BEGIN: bad v_start: %5.3f, must be in [0,1]\n", v_start) > "/dev/stderr"
		err = 1
		exit err
	}
	if(v_end < 0 || v_end > 1){
		printf("ERROR: BEGIN: bad v_end: %5.3f, must be in [0,1]\n", v_end) > "/dev/stderr"
		err = 1
		exit err
	}

	n_colors = "'"$N_COLORS"'" + 0
}
END {
	if(err)
		exit err

	if(n_colors == 1){
		hsv["H"] = h_start
		hsv["S"] = s_start
		hsv["V"] = v_start
		CU_hsv2rgb(hsv, rgb)
		hsv_strs[1] = sprintf("%5.1f,%5.3f,%5.3f", hsv["H"], hsv["S"], hsv["V"])
		rgb_strs[1] = sprintf("%5.3f,%5.3f,%5.3f", rgb["R"], rgb["G"], rgb["B"])
	}else if(h_start <= h_end){
		h_delta = (h_end - h_start)/(n_colors - 1)	# safe as n_colors >= 2
		for(i = 0; i < n_colors; i++){
			alpha = 1.0*((n_colors-1) - i)/(n_colors-1)
			hsv["H"] = h_start + (i * h_delta)
			hsv["S"] = alpha*s_start + (1.0 - alpha)*s_end
			hsv["V"] = alpha*v_start + (1.0 - alpha)*v_end
			CU_hsv2rgb(hsv, rgb)
			hsv_strs[i+1] = sprintf("%5.1f,%5.3f,%5.3f", hsv["H"], hsv["S"], hsv["V"])
			rgb_strs[i+1] = sprintf("%5.3f,%5.3f,%5.3f", rgb["R"], rgb["G"], rgb["B"])
		}
	}else{
		h_delta = (h_start - h_end)/(n_colors - 1)	# safe as n_colors >= 2
		for(i = 0; i < n_colors; i++){
			alpha = 1.0*((n_colors-1) - i)/(n_colors-1)
			hsv["H"] = h_start - (i * h_delta)
			hsv["S"] = alpha*s_start + (1.0 - alpha)*s_end
			hsv["V"] = alpha*v_start + (1.0 - alpha)*v_end
			CU_hsv2rgb(hsv, rgb)
			hsv_strs[i+1] = sprintf("%5.1f,%5.3f,%5.3f", hsv["H"], hsv["S"], hsv["V"])
			rgb_strs[i+1] = sprintf("%5.3f,%5.3f,%5.3f", rgb["R"], rgb["G"], rgb["B"])
		}
	}

	if(fmt == "html"){
		printf("<html>\n")
		printf("<head>\n")
		printf("<meta charset=\"UTF-8\">\n")
		printf("</head>\n")
		printf("<body>\n")
		printf("<table>\n")
		if(grad){
			for(i = 1; i < n_colors; i++){
				cstr = CU_rgb_to_24bit_color(rgb_strs[i])
				cstr1 = CU_rgb_to_24bit_color(rgb_strs[i+1])
				printf("<tr><td style=\"background: linear-gradient(to bottom, #%s, #%s);\">%s, %s</td></tr>\n",
					cstr, cstr1, rgb_strs[i], rgb_strs[i+1])
			}
		}else{
			for(i = 1; i <= n_colors; i++){
				cstr = CU_rgb_to_24bit_color(rgb_strs[i])
				printf("<tr><td style=\"background-color: #%s;\">#%s, %s, %s</td></tr>\n", cstr, cstr, rgb_strs[i], hsv_strs[i])
			}
		}
		printf("</table>\n")
		printf("</body>\n")
		printf("</html>\n")
	}else{
		if(grad){
			printf("%s.values = ", sname)
			for(i = 1; i < n_colors; i++)
				printf("%s%s:%s", i > 1 ? " | " : "", rgb_strs[i], rgb_strs[i+1])
			printf("\n")
		}else{
			printf("%s.values = ", sname)
			for(i = 1; i <= n_colors; i++)
				printf("%s%s", i > 1 ? " | " : "", rgb_strs[i])
			printf("\n")
#			printf("%s.breaks = ", sname)
#			for(i = 1; i < n_colors; i++)
#				printf("%s%s", i > 1 ? " | " : "", i)
#			printf("\n")
		}

	}
	exit err
}' < /dev/null
