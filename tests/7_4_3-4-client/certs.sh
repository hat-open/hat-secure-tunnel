#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

duration=3650
key_type=ec
key_options="-pkeyopt ec_paramgen_curve:prime256v1"
server_key_options="-pkeyopt ec_paramgen_curve:secp224r1"

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type $key_options \
             -days $duration \
             -keyout $DATA_PATH/ca.key \
             -out $DATA_PATH/ca.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type $key_options \
             -days $duration \
             -CA $DATA_PATH/ca.cert \
             -CAkey $DATA_PATH/ca.key \
             -keyout $DATA_PATH/tunnel.key \
             -out $DATA_PATH/tunnel.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type $server_key_options \
             -days $duration \
             -CA $DATA_PATH/ca.cert \
             -CAkey $DATA_PATH/ca.key \
             -keyout $DATA_PATH/server.key \
             -out $DATA_PATH/server.cert
