#!/bin/sh

cd $(dirname "$(realpath "$0")")

find . -name data -type d -exec rm -rf {} +
