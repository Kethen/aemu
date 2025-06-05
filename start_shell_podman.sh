IMAGE_NAME="pspsdk_aemu"

set -xe

if [ "$REBUILD_IMAGE" == "true" ] && podman image exists $IMAGE_NAME
then
	podman image rm -f $IMAGE_NAME
fi

if ! podman image exists $IMAGE_NAME
then
	podman image build -f Dockerfile -t $IMAGE_NAME
fi

mkdir -p workdir

podman run \
	--rm -it \
	-v /dev/bus/usb:/dev/bus/usb \
	-v ./:/workdir \
	-w /workdir \
	$IMAGE_NAME
