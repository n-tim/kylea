#!/bin/bash

echo 'starting build'

mkdir -p kylea-build-debug && cd kylea-build-debug

cmake ../kylea -DCMAKE_BUILD_TYPE=debug
make kylea_tests

echo 'running tests'

exec /app/kylea-build-debug/test/kylea_tests