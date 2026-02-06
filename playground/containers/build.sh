#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..
BUILD_PATH=$RUN_PATH/build

rm -rf $BUILD_PATH
mkdir -p $BUILD_PATH

if [ -z "$*" ]; then
    IMAGES="linux/amd64/build-hat-secure-tunnel:debian11
            linux/amd64/build-hat-secure-tunnel:alpine3.23"
else
    IMAGES="$*"
fi

cd $ROOT_PATH

for IMAGE in $IMAGES; do
    BASE=$(echo $IMAGE | cut -d ':' -f 2)
    CONTAINERFILE=$RUN_PATH/containerfiles/$BASE
    PLATFORM=$(dirname $IMAGE)
    IMAGE_ID=$(podman images -q $IMAGE)

    make clean
    podman build --platform $PLATFORM \
                 -f $CONTAINERFILE \
                 -t $IMAGE \
                 .
    if [ -n "$IMAGE_ID" -a "$IMAGE_ID" != "$(podman images -q $IMAGE)" ]; then
        podman rmi $IMAGE_ID
    fi

    mkdir -p $BUILD_PATH/$PLATFORM/$BASE
    podman run --rm \
               --platform $PLATFORM \
               -v $BUILD_PATH/$PLATFORM/$BASE:/hat/build \
               -i $IMAGE /bin/sh - <<EOF
set -e
CFLAGS="-O2" ./configure
make
EOF
done

make clean
