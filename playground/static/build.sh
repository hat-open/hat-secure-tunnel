#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

if [ ! -f $BUILD_PATH/bin/musl-gcc ]; then
    $RUN_PATH/build-musl.sh
fi

if [ ! -f $BUILD_PATH/lib/libjansson.a ]; then
    $RUN_PATH/build-jansson.sh
fi

if [ ! -f $BUILD_PATH/lib/libuv.a ]; then
    $RUN_PATH/build-libuv.sh
fi

if [ ! -d $BUILD_PATH/include/linux ]; then
    $RUN_PATH/build-kernel-headers.sh
fi

if [ ! -f $BUILD_PATH/lib64/libssl.a ]; then
    $RUN_PATH/build-openssl.sh
fi

mkdir -p $BUILD_PATH/bin

$CC -I$BUILD_PATH/include -L$BUILD_PATH/lib -L$BUILD_PATH/lib64 \
    -static -O2 \
    -o $BUILD_PATH/bin/hat-iec62351-tunnel $ROOT_PATH/src/*.c \
    -luv -lssl -lcrypto -ljansson
