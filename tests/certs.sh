#!/bin/sh

cd $(dirname "$(realpath "$0")")

find . -maxdepth 2 -mindepth 2 -name certs.sh -type f -exec sh {} \;
