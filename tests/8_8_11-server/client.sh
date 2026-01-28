#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

echo "GET / HTTP/1.1" > $DATA_PATH/early_data.txt

( sleep 2; printf "Q" ) | $OPENSSL s_client -tls1_3 \
                  -connect 127.0.0.1:$TUNNEL_PORT \
                  -cert $DATA_PATH/client.cert \
                  -key $DATA_PATH/client.key \
                  -CAfile $DATA_PATH/ca.cert \
                  -sess_out $DATA_PATH/sess.pem > /dev/null 2>&1

exec $OPENSSL s_client -debug -msg -tls1_3 \
                       -connect 127.0.0.1:$TUNNEL_PORT \
                       -cert $DATA_PATH/client.cert \
                       -key $DATA_PATH/client.key \
                       -CAfile $DATA_PATH/ca.cert \
                       -sess_in $DATA_PATH/sess.pem \
                       -early_data $DATA_PATH/early_data.txt \
                       -keylogfile $DATA_PATH/sslkeys.log \
                       -trace
