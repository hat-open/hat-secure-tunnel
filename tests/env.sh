: ${PLAYGROUND_PATH:?}

ROOT_PATH=$PLAYGROUND_PATH/..
BUILD_PATH=$ROOT_PATH/build

OPENSSL1=openssl-1.1
OPENSSL3=openssl

# TUNNEL=$ROOT_PATH/playground/dist/linux/amd64/alpine3.23/hat-iec62351-tunnel
TUNNEL=$BUILD_PATH/hat-iec62351-tunnel

CREATE_CRL=$ROOT_PATH/playgroud/create-crl/create-crl
