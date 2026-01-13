#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..

CACHE_PATH=$RUN_PATH/cache
BUILD_PATH=$RUN_PATH/build

mkdir -p $CACHE_PATH
mkdir -p $BUILD_PATH

JANSSON_URL=https://github.com/akheron/jansson/releases/download/v2.14.1/jansson-2.14.1.tar.gz
JANSSON_FILE=$(basename $JANSSON_URL)
JANSSON_DIR=$BUILD_PATH/$(basename $JANSSON_URL .tar.gz)

# curl -L -o $CACHE_PATH/$JANSSON_FILE $JANSSON_URL

rm -rf $JANSSON_DIR
tar -xzf $CACHE_PATH/$JANSSON_FILE -C $BUILD_PATH

(
cd $JANSSON_DIR
./configure --prefix=$BUILD_PATH
make
make install
)
