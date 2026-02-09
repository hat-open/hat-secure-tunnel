#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

# Certificate validity in days (~10 years) to avoid expiry during tests.
duration=3650


key_type=rsa:2048

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -keyout $DATA_PATH/ca.key \
             -out $DATA_PATH/ca.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -CA $DATA_PATH/ca.cert \
             -CAkey $DATA_PATH/ca.key \
             -keyout $DATA_PATH/tunnel.key \
             -out $DATA_PATH/tunnel.cert

$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -CA $DATA_PATH/ca.cert \
             -CAkey $DATA_PATH/ca.key \
             -keyout $DATA_PATH/server.key \
             -out $DATA_PATH/server.cert

# Test-only: issue a cert used only to populate the CRL.
$OPENSSL req -batch -x509 -noenc \
             -newkey $key_type \
             -days $duration \
             -CA $DATA_PATH/ca.cert \
             -CAkey $DATA_PATH/ca.key \
             -keyout $DATA_PATH/revoked.key \
             -out $DATA_PATH/revoked.cert

# Test-only: generate a short-lived CRL for expiry testing.
$CREATE_CRL -u 12 $DATA_PATH/ca.key \
            <$DATA_PATH/revoked.cert \
            >$DATA_PATH/crl.pem
