#!/bin/sh

set -e

ROOT_PATH=$(dirname "$(realpath "$0")")

STATIC_PATH=$ROOT_PATH/static
BUILD_PATH=$ROOT_PATH/build

$ROOT_PATH/static-build.sh
$ROOT_PATH/tests/clean.sh

dist_name=hat-iec62351-tunnel-$(cat $ROOT_PATH/VERSION)
dist_dir=$BUILD_PATH/$dist_name
dist_file=$dist_dir.tar.gz

dist_test_name=hat-iec62351-tunnel-test-$(cat $ROOT_PATH/VERSION)
dist_test_dir=$BUILD_PATH/$dist_test_name
dist_test_file=$dist_test_dir.tar.gz


rm -rf $dist_dir $dist_file $dist_test_dir $dist_test_file
trap "exit 1" HUP INT PIPE QUIT TERM
trap "rm -rf $dist_dir $dist_test_dir" EXIT


mkdir -p $dist_dir

install -D -m 755 -t $dist_dir/bin \
                     $STATIC_PATH/bin/hat-iec62351-tunnel
install -D -m 644 -t $dist_dir/share/man/man1 \
                     $STATIC_PATH/share/man/man1/hat-iec62351-tunnel.1.gz
install -D -m 644 -t $dist_dir/share/licenses/jansson \
                     $STATIC_PATH/src/jansson-*/LICENSE
install -D -m 644 -t $dist_dir/share/licenses/libuv \
                     $STATIC_PATH/src/libuv-*/LICENSE
install -D -m 644 -t $dist_dir/share/licenses/openssl \
                     $STATIC_PATH/src/openssl-*/LICENSE.txt
install -D -m 644 -t $dist_dir/share/licenses/hat-iec62351-tunnel \
                     $ROOT_PATH/LICENSE
install -D -m 644 -t $dist_dir \
                     $ROOT_PATH/LICENSE

cat >$dist_dir/README <<EOF
hat-iec62351-tunnel - IEC 62351-3 tunnel
========================================

Application providing tunnel between unencrypted and TLS communication
based on IEC 62351-3.

To use hat-iec62351-tunnel and access manual pages, add bin directory to
PATH environment variable.

For more information run:
  $ man hat-iec62351-tunnel
EOF

tar -czf $dist_file --owner=0 --group=0 -C $(dirname $dist_dir) \
                                           $(basename $dist_dir)


mv $dist_dir $dist_test_dir

install -D -m 755 -t $dist_test_dir/bin \
                     $STATIC_PATH/bin/create-crl \
                     $STATIC_PATH/bin/openssl

cp -r $ROOT_PATH/tests $dist_test_dir/tests

cat >$dist_test_dir/tests/env.sh <<EOF
: \${TESTS_PATH:?}

ROOT_PATH=\$TESTS_PATH/..

OPENSSL=\$ROOT_PATH/bin/openssl
TUNNEL=\$ROOT_PATH/bin/hat-iec62351-tunnel
CREATE_CRL=\$ROOT_PATH/bin/create-crl
EOF

tar -czf $dist_test_file --owner=0 --group=0 -C $(dirname $dist_test_dir) \
                                                $(basename $dist_test_dir)
