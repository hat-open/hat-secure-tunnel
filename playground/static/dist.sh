#!/bin/sh

set -e

usage() {
    cat <<EOF
Usage: $0 [OPTION]...

Create static binary distribution.

Options:
  -h, --help              display this help and exit
      --name=NAME         distribution name [hat-secure-tunnel]
      --version=VERSION   version [detected]
      --license=PATH      path to license file [detected]
      --build-dir=PATH    build directory path [build]
      --cache-dir=PATH    cache directory path [cache]
      --output-dir=PATH   output directory path [dist]
EOF
}

RUN_PATH=$(dirname "$(realpath "$0")")

PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..

name=hat-secure-tunnel
version=$(cat $ROOT_PATH/VERSION)
license=$ROOT_PATH/LICENSE
build_dir=$RUN_PATH/build
cache_dir=$RUN_PATH/cache
output_dir=$RUN_PATH/dist

for arg; do
    case "$arg" in
        --help|-h) usage; exit 0;;
        --name=*) name=${arg#*=};;
        --version=*) version=${arg#*=};;
        --license=*) license=${arg#*=};;
        --build-dir=*) build_dir=${arg#*=};;
        --cache-dir=*) cache_dir=${arg#*=};;
        --output-dir=*) output_dir=${arg#*=};;
        * ) echo "$0: unknown option $arg"; exit 1;;
    esac
done

$RUN_PATH/build.sh --name=$name \
                   --build-dir=$build_dir \
                   --cache-dir=$cache_dir

dist_name=$name-$version
dist_dir=$output_dir/$dist_name
dist_file=$dist_dir.tar.gz

rm -rf $dist_dir $dist_file
trap "exit 1" HUP INT PIPE QUIT TERM
trap "rm -rf $dist_dir" EXIT

mkdir -p $dist_dir

install -D -m 755 -t $dist_dir/bin \
                     $build_dir/bin/$name
install -D -m 644 -t $dist_dir/share/man/man1 \
                     $build_dir/share/man/man1/$name.1.gz
install -D -m 644 -t $dist_dir/share/licenses/jansson \
                     $build_dir/src/jansson-*/LICENSE
install -D -m 644 -t $dist_dir/share/licenses/libuv \
                     $build_dir/src/libuv-*/LICENSE
install -D -m 644 -t $dist_dir/share/licenses/openssl \
                     $build_dir/src/openssl-*/LICENSE.txt
install -D -m 644 -t $dist_dir/share/licenses/$name \
                     $license
install -D -m 644 -t $dist_dir \
                     $license

cat >$dist_dir/README <<EOF
$name - IEC 62351-3 tunnel
$(echo $name | tr '[:print:]' '=')=====================

Application providing tunnel between unencrypted and TLS communication
based on IEC 62351-3.

To use $name and access manual pages, add bin directory to
PATH environment variable.

For more information run:
  $ man $name
EOF

tar -czf $dist_file --owner=0 --group=0 -C $(dirname $dist_dir) \
                                           $(basename $dist_dir)

echo "created $dist_file"
