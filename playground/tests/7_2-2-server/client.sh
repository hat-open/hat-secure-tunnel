#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

exec $OPENSSL s_client -debug -msg -tls1_2 \
                       -connect 127.0.0.1:$TUNNEL_PORT \
                       -cert $DATA_PATH/client.cert \
                       -key $DATA_PATH/client.key \
                       -cipher "DHE-RSA-AES256-SHA256"
