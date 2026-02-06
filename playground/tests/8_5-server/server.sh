#!/bin/sh

set -e

RUN_PATH=$(dirname "$(realpath "$0")")
. $RUN_PATH/env.sh

cd $RUN_PATH

exec socat -d2 TCP4-LISTEN:$SERVER_PORT,fork,bind=127.0.0.1 -
