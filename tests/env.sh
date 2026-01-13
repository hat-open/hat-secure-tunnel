: ${TESTS_PATH:?}

ROOT_PATH=$TESTS_PATH/..
BUILD_PATH=$ROOT_PATH/build
PLAYGROUND_PATH=$ROOT_PATH/playground

OPENSSL1_0=openssl-1.0
OPENSSL1_1=openssl-1.1
OPENSSL3=openssl

# TUNNEL=$ROOT_PATH/playground/dist/linux/amd64/alpine3.23/hat-iec62351-tunnel
TUNNEL=$BUILD_PATH/hat-iec62351-tunnel

CREATE_CRL=$PLAYGROUND_PATH/create-crl/create-crl
