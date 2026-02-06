#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

cd $RUN_PATH

cat >$DATA_PATH/conf.json <<EOF
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
                        "ca_path": "$DATA_PATH/ca.cert"
                    },
                    "renegotiate_interval": 1,
                    "session_cache_timeout": 2
                }
            }
        }
    ]
}
EOF

exec $TUNNEL -l DEBUG $DATA_PATH/conf.json
