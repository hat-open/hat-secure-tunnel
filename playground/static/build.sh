#!/bin/sh

# ubuntu dependencies:
# - curl
# - build-essential
# - automake
# - libtool

set -e

usage() {
    cat <<EOF
Usage: $0 [OPTION]...

Build static binaries.

Options:
  -h, --help            display this help and exit
      --name=NAME       main binary name [hat-secure-tunnel]
      --arch=ARCH       target architecture [detected]
      --build-dir=PATH  build directory path [build]
      --cache-dir=PATH  cache directory path [cache]
EOF
    exit 0
}

RUN_PATH=$(dirname "$(realpath "$0")")

PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..

name=hat-secure-tunnel
arch=$(uname -m)
build_dir=$RUN_PATH/build
cache_dir=$RUN_PATH/cache

for arg; do
    case "$arg" in
        --help|-h) usage;;
        --name=*) name=${arg#*=};;
        --arch=*) arch=${arg#*=};;
        --build-dir=*) build_dir=${arg#*=};;
        --cache-dir=*) cache_dir=${arg#*=};;
        *) echo "$0: unknown option $arg"; exit 1;;
    esac
done

download() (
    file=$1
    url=$2
    sum=$3

    mkdir -p $cache_dir
    cd $cache_dir

    if [ ! -f $file ]; then
        curl -L -o $file $url
    fi

    echo "$sum  $file" >$file.sum
    sha256sum -c $file.sum
)

unpack() (
    file=$1

    mkdir -p $build_dir/src
    cd $build_dir/src

    rm -rf ./${file%.tar.gz}
    tar -xzf $cache_dir/$file
)

build_musl() (
    name=$1

    download $name.tar.gz \
             https://musl.libc.org/releases/$name.tar.gz \
             a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4
    unpack $name.tar.gz

    cd $build_dir/src/$name

    ./configure --prefix=$build_dir --syslibdir=$build_dir/lib
    make
    make install
)

build_jansson() (
    name=$1

    download $name.tar.gz \
             https://github.com/akheron/jansson/releases/download/v${name#jansson-}/$name.tar.gz \
             2521cd51a9641d7a4e457f7215a4cd5bb176f690bc11715ddeec483e85d9e2b3
    unpack $name.tar.gz

    cd $build_dir/src/$name

    ./configure --prefix=$build_dir --disable-shared
    make
    make install
)

build_libuv() (
    name=$1

    download $name.tar.gz \
             https://github.com/libuv/libuv/archive/refs/tags/v${name#libuv-}.tar.gz \
             27e55cf7083913bfb6826ca78cde9de7647cded648d35f24163f2d31bb9f51cd
    unpack $name.tar.gz

    cd $build_dir/src/$name

    sh autogen.sh
    ./configure --prefix=$build_dir --disable-shared
    make
    make install
)

build_kernel_headers() (
    name=$1

    download $name.tar.gz \
             https://github.com/sabotage-linux/kernel-headers/archive/refs/tags/v${name#kernel-headers-}.tar.gz \
             16161844e56944d39794ad74c2dfd6faad12bda79b5dc00595f4178d28a92e2d
    unpack $name.tar.gz

    cd $build_dir/src/$name

    make ARCH=$arch prefix=$build_dir install
)

build_openssl() (
    name=$1

    download $name.tar.gz \
             https://github.com/openssl/openssl/releases/download/$name/$name.tar.gz \
             b6a5f44b7eb69e3fa35dbf15524405b44837a481d43d81daddde3ff21fcbb8e9
    unpack $name.tar.gz

    cd $build_dir/src/$name

    ./Configure --prefix=$build_dir \
                no-shared \
                enable-ssl3 \
                enable-ssl3-method \
                enable-weak-ssl-ciphers
                # enable-deprecated \
                # enable-des \
                # enable-legacy \
                # enable-md2 \
    make
    make install
)

build_main() (
    name=$1

    mkdir -p $build_dir/src/$name
    cd $build_dir/src/$name

    $ROOT_PATH/configure --prefix=$build_dir \
                         --name=$name \
                         --without-pkg-config \
                         LDFLAGS="-static"
    make
    make install
)

build_create_crl() (
    $CC -static -g -O2 \
        -o $build_dir/bin/create-crl \
        $PLAYGROUND_PATH/create-crl/main.c \
        -lssl -lcrypto
)

mkdir -p $build_dir

if [ "$arch" = "x86_64" ]; then
    ln -sf $build_dir/lib $build_dir/lib64
fi

if [ ! -f $build_dir/bin/musl-gcc ]; then
    build_musl musl-1.2.5
fi

export CC=$build_dir/bin/musl-gcc

if [ ! -f $build_dir/lib/libjansson.a ]; then
    build_jansson jansson-2.14.1
fi

if [ ! -f $build_dir/lib/libuv.a ]; then
    build_libuv libuv-1.51.0
fi

if [ ! -d $build_dir/include/linux ]; then
    build_kernel_headers kernel-headers-4.19.88-2
fi

if [ ! -f $build_dir/lib/libssl.a ]; then
    build_openssl openssl-3.6.0
fi

build_main $name

build_create_crl
