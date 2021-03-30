#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] [ -b ] -c cfg-file -at { src | dst } db"

if [ -z "$DM_HOME" ] ; then
	LOG ERROR "DM_HOME not defined"
	exit 1
fi

if [ -z "$WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi

DM_SCRIPTS=$DM_HOME/scripts
WM_BIN=$WM_HOME/bin
WM_BUILD=$WM_HOME/src
WM_DEMOS=$WM_HOME/demos
WM_SCRIPTS=$WM_HOME/scripts
WM_DATA=$WM_HOME/data

TMP_DFILE=/tmp/data.$$
TMP_DFILE_2=/tmp/data_2.$$
TMP_AFILE=/tmp/addrs.$$
TMP_ANR_FILE=/tmp/anr_file.$$
TMP_RN_FILE=/tmp/rn_file.$$
TMP_MFILE=/tmp/mf.$$
TMP_PFILE=/tmp/pfile.$$
TMP_JC_FILE=/tmp/mf.$$.json

USE_BUILD=
CFILE=
ATYPE=
DB=

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-b)
		USE_BUILD="yes"
		shift
		;;
	-c)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-c requires cfg-file argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		CFILE=$1
		shift
		;;
	-at)
		shift
		if [ $# -eq 0 ] ; then
			LOG ERROR "-at requires address-type argument"
			echo "$U_MSG" 1>&2
			exit 1
		fi
		ATYPE=$1
		shift
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		DB=$1
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

if [ "$USE_BUILD" == "yes" ] ; then
	BINDIR=$WM_BUILD
else
	BINDIR=$WM_BIN
fi

if [ -z "$CFILE" ] ; then
	LOG ERROR "missing -c cfg-file argument"
	echo "$U_MSG" 1>&2
	exit 1
fi

if [ -z "$ATYPE" ] ; then
	LOG ERROR "missing -at address-type argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ "$ATYPE" != "src" ] && [ "$ATYPE" != "dst" ] ; then
	LOG ERROR "unknown address-type $ATYPE, must src or dst"
	echo "$U_MSG" 1>&2
	exit
fi

# 1. 1st query: get the distinct addrs of type $ATYPE along w/last use and count.
sqlite3 $DB <<_EOF_ > $TMP_DFILE
PRAGMA foreign_keys = ON ;
.headers ON
.mode tabs
SELECT
	MAX(DATE(time_start)) AS last,
	$ATYPE.address,
	'.' AS NA,
	$ATYPE.lng,
	$ATYPE.lat,
	COUNT($ATYPE.address) AS visits
FROM jobs
INNER JOIN addresses AS $ATYPE ON $ATYPE.address_id = ${ATYPE}_addr_id
WHERE $ATYPE.as_reason LIKE 'geo.ok.%'
GROUP BY $ATYPE.address ;
_EOF_

# 2. Put the addresses into their nabes. add_addrs_to_nabes.sh can't read from stdin
tail -n +2 $TMP_DFILE > $TMP_AFILE
$WM_SCRIPTS/add_addrs_to_nabes.sh -sel $WM_DEMOS/select_sn.sh -sf $WM_DATA/Seattle_Neighborhoods/WGS84/Neighborhoods. $TMP_AFILE	|
$WM_SCRIPTS/add_columns.sh -b $TMP_DFILE -mk address											|
$WM_SCRIPTS/extract_cols.sh -c address,nabe,rnum,last,visits > $TMP_ANR_FILE

# 3. 2nd query: get all jobs matching $ATYPE
sqlite3 $DB <<_EOF_ > $TMP_DFILE_2
PRAGMA foreign_keys = ON ;
.headers ON
.mode tabs
SELECT
	DATE(time_start) AS date,
	$ATYPE.address,
	'.' AS NA,
	$ATYPE.lng,
	$ATYPE.lat,
	$ATYPE.rply_address
FROM jobs
INNER JOIN addresses AS $ATYPE ON $ATYPE.address_id = ${ATYPE}_addr_id
WHERE $ATYPE.as_reason LIKE 'geo.ok.%'
ORDER BY $ATYPE.address,date
_EOF_

# 3a.
N_DATES="$(
	sqlite3 $DB <<-_EOF_
	select count(distinct date(time_start)) from jobs ;
_EOF_
)"

# 4. combine data from 1st & 2nd queries along with those nabes w/o visits
#  rnum, prob, fill, title, where title = nabe<br/>visited N dates, last=L<br/>%s
# 
$WM_DEMOS/select_sn.sh $WM_DATA/Seattle_Neighborhoods/WGS84/Neighborhoods.db > $TMP_RN_FILE
$WM_SCRIPTS/add_columns.sh -b $TMP_DFILE_2 -mk address $TMP_ANR_FILE	|
awk -F'\t' '
BEGIN {
	# read file of all nabes.
	rn_file = "'"$TMP_RN_FILE"'"
	for(n_nabes = 0; (getline < rn_file) > 0; ){
		n_nabes++
		if(n_nabes > 1){
			nabes[$2] = $0
			nb_used[$2] = 0
		}
	}
	close(rn_file)
}
NR == 1 {
	hdr = $0
}
NR > 1 {
	if(!($1 in dates)){
		n_dates++
		dates[$1] = 1
	}

	nd = $1"|"$7
	if(!(nd in nb_dates)){
		nb_dates[nd] = 1
		nb_used[$7] = 1
		nb_n_dates[$7]++
		if(nb_n_dates[$7] == 1){
			nb_rnum[$7] = $8
			nb_last[$7] = $1
			nb_visits[$7] = $10
			nb_addr[$7] = $2
		}else if($1 > nb_last[$7]){
			nb_rnum[$7] = $8
			nb_last[$7] = $1
			nb_visits[$7] = $10
			nb_addr[$7] = $2
		}
	}else if($1 > nb_last[$7]){
		nb_last[$7] = $1
		nb_visits[$7] = $10
		nb_addr[$7] = $2
	}
}
END {
	# printf("%d dates\n", n_dates)
	printf("rnum\tnabe\tprob\tdates\tlast\tvisits\taddress\n")
	for(n in nb_n_dates)
		printf("%d\t%s\t%5.3f\t%s\t%s\t%d\t%s\n", nb_rnum[n], n, 1.0*nb_n_dates[n]/n_dates, nb_n_dates[n], nb_last[n], nb_visits[n], nb_addr[n])
	for(n in nabes){
		if(nb_used[n] == 0)
			printf("%s\t%5.3f\t%s\t%s\t%d\t%s\n", nabes[n], 0.0, 0, ".", 0, ".")
		
	}
}'	|
$WM_SCRIPTS/interp_prop.sh -c $CFILE -bk prob -nk fill -meta $TMP_MFILE	|
$WM_SCRIPTS/format_color.sh -k fill					|
awk -F'\t' 'BEGIN {
	n_dates = "'"$N_DATES"'" + 0
}
NR == 1 {
	printf("rnum\tprob\tfill\ttitle\n")
}
NR > 1 {
	printf("%d\t%5.3f\t%s", $1, $3, $8)
	if($4 != 0)
		printf("\t%s:<br/>visited %d/%d dates, last=%s<br>%s\n", $2, $4, n_dates, $5, $7)
	else
		printf("\t%s:<br/>No visits\n", $2)
}' > $TMP_PFILE 

# 5. create the json scale config file
# TODO: data is incomplete
cat $CFILE $TMP_MFILE | $DM_SCRIPTS/cfg_to_json.sh > $TMP_JC_FILE

# 6. create the geojson.  -sc $TMP_JC_FILE is absent b/c bug in shp_to_geojson
$WM_SCRIPTS/extract_cols.sh -hdr drop -c rnum $TMP_PFILE	|
$BINDIR/shp_to_geojson -sc $TMP_JC_FILE -pf $TMP_PFILE -pk rnum -sf $WM_DATA/Seattle_Neighborhoods/WGS84/Neighborhoods.

# 7. Clean up
rm -f $TMP_AFILE $TMP_ANR_FILE $TMP_DFILE $TMP_DFILE_2 $TMP_MFILE $TMP_PFILE $TMP_RN_FILE $TMP_JC_FILE
