#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

LIBUV_URL=https://github.com/libuv/libuv/archive/refs/tags/v1.51.0.tar.gz
LIBUV_FILE=libuv-1.51.0.tar.gz
LIBUV_DIR=$SRC_PATH/${LIBUV_FILE%.tar.gz}

if [ ! -f $CACHE_PATH/$LIBUV_FILE ]; then
    mkdir -p $CACHE_PATH
    curl -L -o $CACHE_PATH/$LIBUV_FILE $LIBUV_URL
fi

rm -rf $LIBUV_DIR
mkdir -p $SRC_PATH
tar -xzf $CACHE_PATH/$LIBUV_FILE -C $SRC_PATH

cd $LIBUV_DIR
sh autogen.sh
./configure --prefix=$BUILD_PATH --disable-shared
make
make install
