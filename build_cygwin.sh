set -xe

IMAGE=pspsdk_aemu_cygwin

if ! podman image exists $IMAGE
then
	podman image build -t $IMAGE -f Dockerfile_cygwin
fi

podman run \
	--rm -it \
	-v ./:/work_dir \
	-w /work_dir \
	--entrypoint '["/bin/bash", "-c"]' \
	$IMAGE \
	'
	set -xe
	rm -rf server_win
	mkdir server_win
	cd server_win
	x86_64-pc-cygwin-gcc -I../sqlite3_win_prebuilt -L../sqlite3_win_prebuilt -I../pspnet_adhocctl_server ../pspnet_adhocctl_server/*.c -Wl,-Bdynamic -lsqlite3 -o aemu_postoffice_server.exe --static
	cp /usr/x86_64-pc-cygwin/sys-root/usr/bin/cygwin1.dll ./
	cp ../sqlite3_win_prebuilt/sqlite3.dll ./
	cp ../pspnet_adhocctl_server/database.db ./
	cp -r ../pspnet_adhocctl_server/www ./
'
