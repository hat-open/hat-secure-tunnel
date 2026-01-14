#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

MUSL_URL=https://musl.libc.org/releases/musl-1.2.5.tar.gz
MUSL_FILE=$(basename $MUSL_URL)
MUSL_DIR=$SRC_PATH/$(basename $MUSL_URL .tar.gz)

if [ ! -f $CACHE_PATH/$MUSL_FILE ]; then
    mkdir -p $CACHE_PATH
    curl -L -o $CACHE_PATH/$MUSL_FILE $MUSL_URL
fi

rm -rf $MUSL_DIR
mkdir -p $SRC_PATH
tar -xzf $CACHE_PATH/$MUSL_FILE -C $SRC_PATH

cd $MUSL_DIR
export CC=cc
./configure --prefix=$BUILD_PATH
make
make install
