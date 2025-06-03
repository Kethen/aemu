IMAGE_NAME="pspsdk_aemu"

if [ "$REBUILD_IMAGE" == "true" ] && podman image exists $IMAGE_NAME
then
	podman image rm -f $IMAGE_NAME
fi

if ! podman image exists $IMAGE_NAME
then
	podman image build -f Dockerfile -t $IMAGE_NAME
fi

podman run \
	--rm -it \
	--security-opt label=disable \
	-v ./:/workdir \
	-v ./build_podman.sh:/workdir/build_podman.sh:ro \
	-v ./script:/workdir/script:ro \
	-w /workdir \
	$IMAGE_NAME \
	/workdir/script
