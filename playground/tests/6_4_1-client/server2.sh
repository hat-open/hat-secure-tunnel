#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

exec $OPENSSL s_server -debug -msg -tls1_2 \
                       -port $SERVER_PORT_2 \
                       -cert $DATA_PATH/server2.cert \
                       -key $DATA_PATH/server2.key \
                       -verifyCAfile $DATA_PATH/tunnel.cert \
                       -Verify 1
