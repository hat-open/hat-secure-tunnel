#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

exec $OPENSSL s_server -debug -msg -tls1_2 \
					   -cipher 'DEFAULT:@SECLEVEL=0' \
                       -port $SERVER_PORT \
                       -cert $DATA_PATH/server.cert \
                       -key $DATA_PATH/server.key \
                       -verifyCAfile $DATA_PATH/tunnel.cert \
                       -Verify 1
