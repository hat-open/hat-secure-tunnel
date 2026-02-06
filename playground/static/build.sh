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
  -h, --help        display this help and exit
      --name=NAME   main binary name [hat-secure-tunnel]
      --arch=ARCH   target architecture [detected]
EOF
    exit 0
}

RUN_PATH=$(dirname "$(realpath "$0")")

PLAYGROUND_PATH=$RUN_PATH/..
ROOT_PATH=$PLAYGROUND_PATH/..
BUILD_PATH=$RUN_PATH/build
CACHE_PATH=$RUN_PATH/cache

name=hat-secure-tunnel
arch=$(uname -m)

for arg ; do
    case "$arg" in
        --help|-h) usage ;;
        --name=*) name=${arg#*=} ;;
        --arch=*) arch=${arg#*=} ;;
        * ) echo "$0: unknown option $arg"; exit 1 ;;
    esac
done

download() (
    file=$1
    url=$2
    sum=$3

    mkdir -p $CACHE_PATH
    cd $CACHE_PATH

    if [ ! -f $file ]; then
        curl -L -o $file $url
    fi

    echo "$sum  $file" >$file.sum
    sha256sum -c $file.sum
)

unpack() (
    file=$1

    mkdir -p $BUILD_PATH/src
    cd $BUILD_PATH/src

    rm -rf ./${file%.tar.gz}
    tar -xzf $CACHE_PATH/$file
)

build_musl() (
    name=$1

    download $name.tar.gz \
             https://musl.libc.org/releases/$name.tar.gz \
             a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4
    unpack $name.tar.gz

    cd $BUILD_PATH/src/$name

    ./configure --prefix=$BUILD_PATH --syslibdir=$BUILD_PATH/lib
    make
    make install
)

build_jansson() (
    name=$1

    download $name.tar.gz \
             https://github.com/akheron/jansson/releases/download/v${name#jansson-}/$name.tar.gz \
             2521cd51a9641d7a4e457f7215a4cd5bb176f690bc11715ddeec483e85d9e2b3
    unpack $name.tar.gz

    cd $BUILD_PATH/src/$name

    ./configure --prefix=$BUILD_PATH --disable-shared
    make
    make install
)

build_libuv() (
    name=$1

    download $name.tar.gz \
             https://github.com/libuv/libuv/archive/refs/tags/v${name#libuv-}.tar.gz \
             27e55cf7083913bfb6826ca78cde9de7647cded648d35f24163f2d31bb9f51cd
    unpack $name.tar.gz

    cd $BUILD_PATH/src/$name

    sh autogen.sh
    ./configure --prefix=$BUILD_PATH --disable-shared
    make
    make install
)

build_kernel_headers() (
    name=$1

    download $name.tar.gz \
             https://github.com/sabotage-linux/kernel-headers/archive/refs/tags/v${name#kernel-headers-}.tar.gz \
             16161844e56944d39794ad74c2dfd6faad12bda79b5dc00595f4178d28a92e2d
    unpack $name.tar.gz

    cd $BUILD_PATH/src/$name

    make ARCH=$arch prefix=$BUILD_PATH install
)

build_openssl() (
    name=$1

    download $name.tar.gz \
             https://github.com/openssl/openssl/releases/download/$name/$name.tar.gz \
             b6a5f44b7eb69e3fa35dbf15524405b44837a481d43d81daddde3ff21fcbb8e9
    unpack $name.tar.gz

    cd $BUILD_PATH/src/$name

    ./Configure --prefix=$BUILD_PATH \
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

    mkdir -p $BUILD_PATH/src/$name
    cd $BUILD_PATH/src/$name

    $ROOT_PATH/configure --prefix=$BUILD_PATH \
                         --name=$name \
                         --without-pkg-config \
                         CFLAGS="-static"
    make
    make install
)

build_create_crl() (
    $CC -static -g -O2 \
        -o $BUILD_PATH/bin/create-crl \
        $PLAYGROUND_PATH/create-crl/main.c \
        -lssl -lcrypto
)

mkdir -p $BUILD_PATH

if [ "$arch" = "x86_64" ]; then
    ln -sf $BUILD_PATH/lib $BUILD_PATH/lib64
fi

if [ ! -f $BUILD_PATH/bin/musl-gcc ]; then
    build_musl musl-1.2.5
fi

export CC=$BUILD_PATH/bin/musl-gcc

if [ ! -f $BUILD_PATH/lib/libjansson.a ]; then
    build_jansson jansson-2.14.1
fi

if [ ! -f $BUILD_PATH/lib/libuv.a ]; then
    build_libuv libuv-1.51.0
fi

if [ ! -d $BUILD_PATH/include/linux ]; then
    build_kernel_headers kernel-headers-4.19.88-2
fi

if [ ! -f $BUILD_PATH/lib/libssl.a ]; then
    build_openssl openssl-3.6.0
fi

build_main $name

build_create_crl
