#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

cd $RUN_PATH


cert_sn() {
    $OPENSSL x509 -noout -serial -in $1 | sed 's/^serial=//'
}

cat >$DATA_PATH/conf.json <<EOF
{
    "mappings": [
        {
            "server": {
                "host": "127.0.0.1",
                "port": $TUNNEL_PORT_1
            },
            "client": {
                "host": "127.0.0.1",
                "port": $SERVER_PORT_1,
                "ssl": {
                    "local_cert": {
                        "cert_path": "$DATA_PATH/tunnel.cert",
                        "key_path": "$DATA_PATH/tunnel.key"
                    },
                    "remote_cert": {
                        "ca_path": "$DATA_PATH/ca.cert",
                        "allowed_cert_sn": ["$(cert_sn $DATA_PATH/server1.cert)"]
                    }
                }
            }
        },
        {
            "server": {
                "host": "127.0.0.1",
                "port": $TUNNEL_PORT_2
            },
            "client": {
                "host": "127.0.0.1",
                "port": $SERVER_PORT_2,
                "ssl": {
                    "local_cert": {
                        "cert_path": "$DATA_PATH/tunnel.cert",
                        "key_path": "$DATA_PATH/tunnel.key"
                    },
                    "remote_cert": {
                        "ca_path": "$DATA_PATH/ca.cert",
                        "allowed_cert_sn": ["$(cert_sn $DATA_PATH/tunnel.cert)"]
                    }
                }
            }
        }
    ]
}
EOF

exec $TUNNEL $DATA_PATH/conf.json
