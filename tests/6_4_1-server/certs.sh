#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

duration=3650
key_type=rsa:2048

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -keyout $DATA_PATH/ca_tunnel.key \
          -out $DATA_PATH/ca_tunnel.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -keyout $DATA_PATH/ca_client1.key \
          -out $DATA_PATH/ca_client1.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -keyout $DATA_PATH/ca_client2.key \
          -out $DATA_PATH/ca_client2.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -keyout $DATA_PATH/ca_client3.key \
          -out $DATA_PATH/ca_client3.cert

cat $DATA_PATH/ca_client1.cert $DATA_PATH/ca_client2.cert >$DATA_PATH/ca_all.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -CA $DATA_PATH/ca_tunnel.cert \
          -CAkey $DATA_PATH/ca_tunnel.key \
          -keyout $DATA_PATH/tunnel.key \
          -out $DATA_PATH/tunnel.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -CA $DATA_PATH/ca_client1.cert \
          -CAkey $DATA_PATH/ca_client1.key \
          -keyout $DATA_PATH/client1.key \
          -out $DATA_PATH/client1.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -CA $DATA_PATH/ca_client2.cert \
          -CAkey $DATA_PATH/ca_client2.key \
          -keyout $DATA_PATH/client2.key \
          -out $DATA_PATH/client2.cert

$OPENSSL3 req -batch -x509 -noenc \
          -newkey $key_type \
          -days $duration \
          -CA $DATA_PATH/ca_client3.cert \
          -CAkey $DATA_PATH/ca_client3.key \
          -keyout $DATA_PATH/client3.key \
          -out $DATA_PATH/client3.cert
