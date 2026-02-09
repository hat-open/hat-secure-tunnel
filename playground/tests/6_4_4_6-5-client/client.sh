#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

cd $RUN_PATH

# Keep stdin open so the TLS handshake completes in non-interactive runs.
exec sh -c "tail -f /dev/null | socat -d2 TCP4-CONNECT:127.0.0.1:$TUNNEL_PORT -"
