#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

duration=3650
key_type=rsa:2048
client_key_type=rsa:1024

openssl req -batch -x509 -noenc \
        -newkey $key_type \
        -days $duration \
        -keyout $DATA_PATH/ca.key \
        -out $DATA_PATH/ca.cert

openssl req -batch -x509 -noenc \
        -newkey $key_type \
        -days $duration \
        -CA $DATA_PATH/ca.cert \
        -CAkey $DATA_PATH/ca.key \
        -keyout $DATA_PATH/tunnel.key \
        -out $DATA_PATH/tunnel.cert

openssl req -batch -x509 -noenc \
        -newkey $client_key_type \
        -days $duration \
        -CA $DATA_PATH/ca.cert \
        -CAkey $DATA_PATH/ca.key \
        -keyout $DATA_PATH/client.key \
        -out $DATA_PATH/client.cert
