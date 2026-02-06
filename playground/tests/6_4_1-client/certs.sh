#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

duration=3650
key_type=rsa:2048

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -keyout $DATA_PATH/ca_tunnel.key \
             -out $DATA_PATH/ca_tunnel.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -keyout $DATA_PATH/ca_server1.key \
             -out $DATA_PATH/ca_server1.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -keyout $DATA_PATH/ca_server2.key \
             -out $DATA_PATH/ca_server2.cert

cat $DATA_PATH/ca_server1.cert >$DATA_PATH/ca_all.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -CA $DATA_PATH/ca_tunnel.cert \
             -CAkey $DATA_PATH/ca_tunnel.key \
             -keyout $DATA_PATH/tunnel.key \
             -out $DATA_PATH/tunnel.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -CA $DATA_PATH/ca_server1.cert \
             -CAkey $DATA_PATH/ca_server1.key \
             -keyout $DATA_PATH/server1.key \
             -out $DATA_PATH/server1.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -CA $DATA_PATH/ca_server2.cert \
             -CAkey $DATA_PATH/ca_server2.key \
             -keyout $DATA_PATH/server2.key \
             -out $DATA_PATH/server2.cert
