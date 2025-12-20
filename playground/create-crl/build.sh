#!/bin/bash

set -e

cd $(dirname "$(realpath "$0")")

cc -o create-crl main.c -lssl -lcrypto
