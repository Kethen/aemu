podman run --rm -it -v ./:/workdir:ro -w /workdir --net host node:lts-alpine3.22 node server.js
