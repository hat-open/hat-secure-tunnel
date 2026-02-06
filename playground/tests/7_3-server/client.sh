#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

# HACK
OPENSSL=openssl-1.0

exec $OPENSSL s_client -debug -msg -tls1_2 \
                       -connect 127.0.0.1:$TUNNEL_PORT \
                       -cert $DATA_PATH/client.cert \
                       -key $DATA_PATH/client.key \
                       -cipher "RC4-MD5:@SECLEVEL=0"
