#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

OPENSSL_URL=https://github.com/openssl/openssl/releases/download/openssl-3.6.0/openssl-3.6.0.tar.gz
OPENSSL_FILE=$(basename $OPENSSL_URL)
OPENSSL_DIR=$SRC_PATH/$(basename $OPENSSL_URL .tar.gz)

if [ ! -f $CACHE_PATH/$OPENSSL_FILE ]; then
    mkdir -p $CACHE_PATH
    curl -L -o $CACHE_PATH/$OPENSSL_FILE $OPENSSL_URL
fi

rm -rf $OPENSSL_DIR
mkdir -p $SRC_PATH
tar -xzf $CACHE_PATH/$OPENSSL_FILE -C $SRC_PATH

cd $OPENSSL_DIR
./Configure --prefix=$BUILD_PATH no-shared
make
make install
