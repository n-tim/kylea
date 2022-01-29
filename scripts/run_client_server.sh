#!/bin/bash

echo 'running client and server'
exec /opt/app/kylea-build-debug/src/server/server 127.0.0.1 567 &
exec /opt/app/kylea-build-debug/src/client/client 127.0.0.1 567 0