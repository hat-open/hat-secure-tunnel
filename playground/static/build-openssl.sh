#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..

CACHE_PATH=$RUN_PATH/cache
BUILD_PATH=$RUN_PATH/build

mkdir -p $CACHE_PATH
mkdir -p $BUILD_PATH

OPENSSL_URL=https://github.com/openssl/openssl/releases/download/openssl-3.6.0/openssl-3.6.0.tar.gz
OPENSSL_FILE=$(basename $OPENSSL_URL)
OPENSSL_DIR=$BUILD_PATH/$(basename $OPENSSL_URL .tar.gz)

# curl -L -o $CACHE_PATH/$OPENSSL_FILE $OPENSSL_URL

rm -rf $OPENSSL_DIR
tar -xzf $CACHE_PATH/$OPENSSL_FILE -C $BUILD_PATH

(
cd $OPENSSL_DIR
./Configure --prefix=$BUILD_PATH
make
make install
)
