#!/bin/sh

cd $(dirname "$(realpath "$0")")

find . -maxdepth 2 -mindepth 2 -name TODO -type f -exec \
    sh -c 'printf "$(basename $(dirname {})):\\n"; cat {} | sed "s/^/    /"; echo' \;
