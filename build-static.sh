#!/bin/sh

# ubuntu dependencies:
# - curl
# - build-essential
# - automake
# - libtool

set -e

ROOT_PATH=$(dirname "$(realpath "$0")")

STATIC_PATH=$ROOT_PATH/static
CACHE_PATH=$ROOT_PATH/cache
PLAYGROUND_PATH=$ROOT_PATH/playground

arch=$(uname -m)


download () (
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


unpack () (
    file=$1

    mkdir -p $STATIC_PATH/src
    cd $STATIC_PATH/src

    rm -rf ./${file%.tar.gz}
    tar -xzf $CACHE_PATH/$file
)


build_musl () (
    name=$1

    download $name.tar.gz \
             https://musl.libc.org/releases/$name.tar.gz \
             a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4
    unpack $name.tar.gz

    cd $STATIC_PATH/src/$name

    ./configure --prefix=$STATIC_PATH --syslibdir=$STATIC_PATH/lib
    make
    make install
)


build_jansson () (
    name=$1

    download $name.tar.gz \
             https://github.com/akheron/jansson/releases/download/v${name#jansson-}/$name.tar.gz \
             2521cd51a9641d7a4e457f7215a4cd5bb176f690bc11715ddeec483e85d9e2b3
    unpack $name.tar.gz

    cd $STATIC_PATH/src/$name

    ./configure --prefix=$STATIC_PATH --disable-shared
    make
    make install
)


build_libuv () (
    name=$1

    download $name.tar.gz \
             https://github.com/libuv/libuv/archive/refs/tags/v${name#libuv-}.tar.gz \
             27e55cf7083913bfb6826ca78cde9de7647cded648d35f24163f2d31bb9f51cd
    unpack $name.tar.gz

    cd $STATIC_PATH/src/$name

    sh autogen.sh
    ./configure --prefix=$STATIC_PATH --disable-shared
    make
    make install
)


build_kernel_headers () (
    name=$1

    download $name.tar.gz \
             https://github.com/sabotage-linux/kernel-headers/archive/refs/tags/v${name#kernel-headers-}.tar.gz \
             16161844e56944d39794ad74c2dfd6faad12bda79b5dc00595f4178d28a92e2d
    unpack $name.tar.gz

    cd $STATIC_PATH/src/$name

    make ARCH=$arch prefix=$STATIC_PATH install
)


build_openssl () (
    name=$1

    download $name.tar.gz \
             https://github.com/openssl/openssl/releases/download/$name/$name.tar.gz \
             b6a5f44b7eb69e3fa35dbf15524405b44837a481d43d81daddde3ff21fcbb8e9
    unpack $name.tar.gz

    cd $STATIC_PATH/src/$name

    ./Configure --prefix=$STATIC_PATH \
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


build_hat_iec62351_tunnel () (
    name=hat-iec62351-tunnel

    mkdir -p $STATIC_PATH/src/$name
    cd $STATIC_PATH/src/$name

    $ROOT_PATH/configure --prefix=$STATIC_PATH \
                         --without-pkg-config \
                         CFLAGS="-static"
    make
    make install
)


build_create_crl () (
    $CC -static -g -O2 \
        -o $STATIC_PATH/bin/create-crl \
        $PLAYGROUND_PATH/create-crl/main.c \
        -lssl -lcrypto
)


mkdir -p $STATIC_PATH

if [ "$arch" = "x86_64" ]; then
    if [ ! -L $STATIC_PATH/lib64 ]; then
        ln -sf $STATIC_PATH/lib $STATIC_PATH/lib64
    fi
fi

if [ ! -f $STATIC_PATH/bin/musl-gcc ]; then
    build_musl musl-1.2.5
fi

export CC=$STATIC_PATH/bin/musl-gcc

if [ ! -f $STATIC_PATH/lib/libjansson.a ]; then
    build_jansson jansson-2.14.1
fi

if [ ! -f $STATIC_PATH/lib/libuv.a ]; then
    build_libuv libuv-1.51.0
fi

if [ ! -d $STATIC_PATH/include/linux ]; then
    build_kernel_headers kernel-headers-4.19.88-2
fi

if [ ! -f $STATIC_PATH/lib/libssl.a ]; then
    build_openssl openssl-3.6.0
fi

build_hat_iec62351_tunnel

build_create_crl
