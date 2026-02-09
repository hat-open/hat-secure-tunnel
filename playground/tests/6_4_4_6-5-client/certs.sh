#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh
export DATA_PATH

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

$OPENSSL req -batch -noenc \
             -newkey $key_type \
             -keyout $DATA_PATH/server.key \
             -out $DATA_PATH/server.csr

$OPENSSL x509 -req \
             -days $duration \
             -in $DATA_PATH/server.csr \
             -CA $DATA_PATH/ca.cert \
             -CAkey $DATA_PATH/ca.key \
             -CAcreateserial \
             -out $DATA_PATH/server.cert

$OPENSSL x509 -in $DATA_PATH/server.cert -outform der -out $DATA_PATH/server.der

# Test-only: flip a byte in the DER signature to make it invalid.
python3 - <<'PY'
import os
from pathlib import Path

p = Path(os.environ["DATA_PATH"]) / "server.der"
data = bytearray(p.read_bytes())
# Flip the last byte in the signature to break verification.
data[-1] ^= 0xFF
p.write_bytes(data)
PY

$OPENSSL x509 -inform der \
             -in $DATA_PATH/server.der \
             -out $DATA_PATH/server.bad.cert
