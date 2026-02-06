: ${TESTS_PATH:?}

PLAYGROUND_PATH=$TESTS_PATH/..
STATIC_PATH=$PLAYGROUND_PATH/static

OPENSSL=$STATIC_PATH/build/bin/openssl
TUNNEL=$STATIC_PATH/build/bin/hat-secure-tunnel
CREATE_CRL=$STATIC_PATH/build/bin/create-crl
