#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..

CACHE_PATH=$RUN_PATH/cache
BUILD_PATH=$RUN_PATH/build

mkdir -p $CACHE_PATH
mkdir -p $BUILD_PATH

LIBUV_URL=https://github.com/libuv/libuv/archive/refs/tags/v1.51.0.tar.gz
LIBUV_FILE=libuv-1.51.0.tar.gz
LIBUV_DIR=$BUILD_PATH/${LIBUV_FILE%.tar.gz}

# curl -L -o $CACHE_PATH/$LIBUV_FILE $LIBUV_URL

rm -rf $LIBUV_DIR
tar -xzf $CACHE_PATH/$LIBUV_FILE -C $BUILD_PATH

(
cd $LIBUV_DIR
sh autogen.sh
./configure --prefix=$BUILD_PATH
make
make install
)
