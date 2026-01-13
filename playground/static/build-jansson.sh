#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

JANSSON_URL=https://github.com/akheron/jansson/releases/download/v2.14.1/jansson-2.14.1.tar.gz
JANSSON_FILE=$(basename $JANSSON_URL)
JANSSON_DIR=$SRC_PATH/$(basename $JANSSON_URL .tar.gz)

if [ ! -f $CACHE_PATH/$JANSSON_FILE ]; then
    mkdir -p $CACHE_PATH
    curl -L -o $CACHE_PATH/$JANSSON_FILE $JANSSON_URL
fi

rm -rf $JANSSON_DIR
mkdir -p $SRC_PATH
tar -xzf $CACHE_PATH/$JANSSON_FILE -C $SRC_PATH

cd $JANSSON_DIR
./configure --prefix=$BUILD_PATH
make
make install
