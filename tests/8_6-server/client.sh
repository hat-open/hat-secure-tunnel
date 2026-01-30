#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

( sleep 2; echo "C"; cat ) | \
$OPENSSL s_client -debug -msg -tls1_3 \
                  -connect 127.0.0.1:$TUNNEL_PORT \
                  -cert $DATA_PATH/client.cert \
                  -key $DATA_PATH/client.key \
                  -keylogfile $DATA_PATH/sslkeys.log

