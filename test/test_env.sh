export OPENSSL_DIR=$PWD/../../openssl
export CURL_DIR=$PWD/../../curl
export ACVP_DIR=$PWD/../src/.libs
export CRITERION_DIR=/usr/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OPENSSL_DIR/lib:$CURL_DIR/lib:$ACVP_DIR/lib:$CRITERION_DIR/lib
