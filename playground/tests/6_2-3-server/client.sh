#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

printf "Q\n" | $OPENSSL s_client -debug -msg -tls1_2 \
                                 -connect 127.0.0.1:$TUNNEL_PORT \
                                 -cert $DATA_PATH/client.cert \
                                 -key $DATA_PATH/client.key \
                                 -sess_out $DATA_PATH/session.pem

$OPENSSL s_client -debug -msg -tls1_3 \
                  -connect 127.0.0.1:$TUNNEL_PORT \
                  -cert $DATA_PATH/client.cert \
                  -key $DATA_PATH/client.key \
                  -keylogfile $DATA_PATH/sslkeys.log \
                  -sess_in $DATA_PATH/session.pem
