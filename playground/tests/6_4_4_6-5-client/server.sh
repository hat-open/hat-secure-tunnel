#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

# Keep stdin open so s_server keeps accepting connections in non-interactive runs.
exec sh -c "tail -f /dev/null | $OPENSSL s_server -debug -msg -tls1_2 \
                       -cipher 'DEFAULT:@SECLEVEL=0' \
                       -port $SERVER_PORT \
                       -cert $DATA_PATH/server.bad.cert \
                       -key $DATA_PATH/server.key \
                       -verifyCAfile $DATA_PATH/tunnel.cert \
                       -Verify 1"
