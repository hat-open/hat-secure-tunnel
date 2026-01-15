#!/bin/sh

cd $(dirname "$(realpath "$0")")

find . -maxdepth 2 -mindepth 2 -name data -type d -exec rm -rf {} +
