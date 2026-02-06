#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

exec $OPENSSL s_client -debug -msg -tls1_3 \
                       -connect 127.0.0.1:$TUNNEL_PORT \
                       -cert $DATA_PATH/ca.cert \
                       -key $DATA_PATH/ca.key \
                       -requestCAfile $DATA_PATH/ca_required.cert \
                       -trace
