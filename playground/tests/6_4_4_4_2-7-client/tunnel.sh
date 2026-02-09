#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

cd $RUN_PATH

# Ensure a CRL exists; reuse existing inputs when possible or regenerate.
if [ ! -f "$DATA_PATH/crl.pem" ]; then
    if [ -f "$DATA_PATH/ca.key" ] && [ -f "$DATA_PATH/revoked.cert" ]; then
        $CREATE_CRL -u 3600 $DATA_PATH/ca.key \
            <$DATA_PATH/revoked.cert \
            >$DATA_PATH/crl.pem
    else
        $RUN_PATH/certs.sh
    fi
fi

cat >$DATA_PATH/conf.json <<EOT
{
    "mappings": [
        {
            "server": {
                "host": "127.0.0.1",
                "port": $TUNNEL_PORT
            },
            "client": {
                "host": "127.0.0.1",
                "port": $SERVER_PORT,
                "ssl": {
                    "local_cert": {
                        "cert_path": "$DATA_PATH/tunnel.cert",
                        "key_path": "$DATA_PATH/tunnel.key"
                    },
                    "remote_cert": {
                        "ca_path": "$DATA_PATH/ca.cert",
                        "crl": {
                            "path": "$DATA_PATH/crl.pem",
                            "update_interval": 5
                        }
                    },
                    "renegotiate_interval": 5
                }
            }
        }
    ]
}
EOT

# Remove the CRL after a delay to simulate it becoming inaccessible.
(
    sleep 8
    rm -f "$DATA_PATH/crl.pem"
) &

exec $TUNNEL $DATA_PATH/conf.json
