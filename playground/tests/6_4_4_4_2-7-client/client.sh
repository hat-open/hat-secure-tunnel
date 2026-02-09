#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

cd $RUN_PATH

# Keep the connection open long enough to observe CRL access warnings, then exit.
exec sh -c "timeout 20 sh -c 'tail -f /dev/null | socat -d2 TCP4-CONNECT:127.0.0.1:$TUNNEL_PORT -'"
