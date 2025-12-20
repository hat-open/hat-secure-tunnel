#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

duration=3650
key_type=rsa:2048

openssl req -batch -x509 -noenc \
        -newkey $key_type \
        -days $duration \
        -keyout $DATA_PATH/ca_tunel.key \
        -out $DATA_PATH/ca_tunel.cert

    openssl req -batch -x509 -noenc \
        -newkey $key_type \
        -days $duration \
        -keyout $DATA_PATH/ca_client.key \
        -out $DATA_PATH/ca_client.cert

openssl req -batch -x509 -noenc \
        -newkey $key_type \
        -days $duration \
        -CA $DATA_PATH/ca_tunel.cert \
        -CAkey $DATA_PATH/ca_tunel.key \
        -keyout $DATA_PATH/tunnel.key \
        -out $DATA_PATH/tunnel.cert

openssl req -batch -x509 -noenc \
        -newkey $key_type \
        -days $duration \
        -CA $DATA_PATH/ca_client.cert \
        -CAkey $DATA_PATH/ca_client.key \
        -keyout $DATA_PATH/client.key \
        -out $DATA_PATH/client.cert
