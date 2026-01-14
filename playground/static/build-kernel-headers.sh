#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

KH_URL=https://github.com/sabotage-linux/kernel-headers/archive/refs/tags/v4.19.88-2.tar.gz
KH_FILE=kernel-headers-4.19.88-2.tar.gz
KH_DIR=$SRC_PATH/${KH_FILE%.tar.gz}

if [ ! -f $CACHE_PATH/$KH_FILE ]; then
    mkdir -p $CACHE_PATH
    curl -L -o $CACHE_PATH/$KH_FILE $KH_URL
fi

rm -rf $KH_DIR
mkdir -p $SRC_PATH
tar -xzf $CACHE_PATH/$KH_FILE -C $SRC_PATH

cd $KH_DIR
make ARCH=x86_64 prefix=$BUILD_PATH install
