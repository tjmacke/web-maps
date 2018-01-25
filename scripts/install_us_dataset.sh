#! /bin/bash
#
. ~/etc/funcs.sh

U_MSG="usage: $0 [ -help ] install-dir"

if [ -z "WM_HOME" ] ; then
	LOG ERROR "WM_HOME not defined"
	exit 1
fi

WM_BIN=$WM_HOME/bin
WM_DATA=$WM_HOME/data
WM_ZIPS=$WM_DATA/zips

FM_HOME=$HOME/fmap
FM_BIN=$FM_HOME/bin

INS_DIR=
rval=0

while [ $# -gt 0 ] ; do
	case $1 in
	-help)
		echo "$U_MSG"
		exit 0
		;;
	-*)
		LOG ERROR "unknown option $1"
		echo "$U_MSG" 1>&2
		exit 1
		;;
	*)
		INS_DIR=$1
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

if [ -z "$INS_DIR" ] ; then
	LOG ERROR "missing install-dir argument"
	echo "$U_MSG" 1>&2
	exit 1
elif [ -d "$INS_DIR" ] ; then
	LOG ERROR "install dir $INS_DIR must not exist, choose a new name or remove it"
	exit 1
elif [ -f "$INS_DIR" ] ; then
	LOG ERROR "$INS_DIR is a file, choose a new name or remove it"
	exit 1
fi

zpat="$(echo -e "$INS_DIR" |\
	awk -F'_' '{
		if(NF < 3 || NF > 4){
			printf("ERROR: main: bad install dir: %s, must be 3 or 4 fields separated by _\n", $0) > "/dev/stderr"
			err = 1
			exit err
		}
		if($1 != "tl" && $1 != "cb"){
			printf("ERROR: main: $1 must be either \"tl\" or \"cb\"\n") > "/dev/stderr"
			err = 1
			exit err
		}else
			k = $1
		if($2 !~ /^20[0-9][0-9]$/){
			printf("ERROR: main: $2 must be a year beginning with 20\n") > "/dev/stderr"
			err = 1
			exit err
		}else
			y = $2
		if($3 != "place" && $3 != "tract"){
			printf("ERROR: main: $1 must be either \"place\" or \"tract\"\n") > "/dev/stderr"
			err = 1
			exit err
		}else
			t = $3
		if(k == "tl"){
			if(NF == 4){
				printf("ERROR: main: tl data does not support resolution\n") > "/dev/stderr"
				err = 1
				exit err
			}
		}else if($4 == ""){
			printf("ERROR: main: cb data requires resolution\n") > "/dev/stderr"
			err = 1
			exit err
		}else if($4 != "500k" && $4 != "5m" && $4 != "20m"){
			printf("ERROR: main: unknown resolution %s, must be 500, 5m or 20m\n", $4) > "/dev/stderr"
			err = 1
			exit err
		}else
			r = $4
	}
	END {
		if(err)
			printf("%s", "")
		else if(k == "tl")
			printf("%s_%s/%s_%s_??_%s.zip", k, y, k, y, t) 
		else
			printf("%s_%s/%s_%s_??_%s_%s.zip", k, y, k, y, t, r) 
		exit err
	}')"

if [ -z "$zpat" ] ; then
	LOG ERROR "could not parse install-dir $INS_DIR"
	exit 1
fi

zdpat="$(echo "$zpat" | awk -F/ '{ print $1 }')"
if [ ! -d $WM_ZIPS/$zdpat ] ; then
	LOG ERROR "zip file directory $zdpat either is not a directory or is empty"
	exit 1
fi

n_zips=$(ls $WM_ZIPS/$zpat | wc -l)
if [ $n_zips -eq 0 ] ; then
	LOG ERROR "zip pattern $zpat does match any files"
	exit 1
fi

mkdir $INS_DIR
cd $INS_DIR

LOG INFO "unpack $n_zips zipfiles into $INS_DIR"
for z in $WM_ZIPS/$zpat ; do
	unzip -q $z
done
LOG INFO "unpacked $n_zips zipfiles into $INS_DIR"

LOG INFO "remove *.cpg, *.xml files"
rm *.cpg *.xml
LOG INFO "removed *.cpg, *.xml files"

LOG INFO "convert dbase *.dbf files to sqlite3 *.db files"
for d in *.dbf ; do
	bn=$(basename $d .dbf)
	db=$bn.db
	$WM_BIN/dbase_to_sqlite -t data -pk +rnum -ktl statefp.tsv $d | sqlite3 $db
done
LOG INFO "converted dbase *.dbf files to sqlite3 *.db files"

LOG INFO "create the *.key files from *.db as NAME, STUSAB_PLACEFP"
for d in *.db ; do
	bn=$(basename $d .db)
	kf=$bn.key
	sqlite3 $d <<-_EOF_ > $kf
	SELECT NAME || ', ' || statefp.STUSAB || '_' || PLACEFP from data, statefp where data.STATEFP = statefp.STATEFP ;
	_EOF_
done
LOG INFO "create the *.key files from *.db as NAME, STUSAB_PLACEFP"

LOG INFO "create the md5 hashes of the keys: *.key -> *.md5"
for k in *.key; do
	bn=$(basename $k .key)
	md5=$bn.md5
	$FM_BIN/mk_md5_lines $k > $md5
done
LOG INFO "created the md5 hashes of the keys: *.key -> *.md5"

# the records in this index are shp.hdr + shp.data
LOG INFO "convert the shx files to ridx files"
for r in *.shp; do
	$WM_BIN/index_shp_file $r
done 2> $INS_DIR.nrecs
LOG INFO "converted the shx files to ridx files"

# 2. make filemap
LOG INFO "make the file map $INS_DIR.fmap"
cat > $INS_DIR.fmap <<_EOF_
root = $WM_DATA/$INS_DIR
format = shp
ridx = ridx
key = md5
index = $INS_DIR.i2r
count = $(ls *.shp | wc -l)
files = {
_EOF_
awk 'BEGIN {
	ins_dir = "'"$INS_DIR"'"
}
{
	printf(" %s %d %s\n", ins_dir, NR, $0)
}' $INS_DIR.nrecs >> $INS_DIR.fmap
echo '}' >> $INS_DIR.fmap
LOG INFO "made the file map $INS_DIR.fmap"

# 3. make key index
LOG INFO "make the key index $INS_DIR.i2r"
$FM_BIN/mk_key2rnum $INS_DIR.fmap
LOG INFO "made the key index $INS_DIR.i2r"

# 4. set all files to 444
chmod 444 *

exit $rval
