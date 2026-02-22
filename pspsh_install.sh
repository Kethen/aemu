set -xe

base_path=dist_660_release

if [ "$DEBUG" == "true" ]
then
	base_path=dist_660_debug
fi

for f in $base_path/kd/*
do
	name=$(basename $f)
	pspsh -v -e "cp $f ms0:/kd"
	pspsh -v -e "cp $f ef0:/kd" || true
done

pspsh -v -e "cp $base_path/seplugins/atpro.prx ms0:/seplugins/"
pspsh -v -e "cp $base_path/seplugins/atpro.prx ef0:/seplugins/" || true
