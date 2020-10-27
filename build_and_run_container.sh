#!/bin/bash

docker build -t kylea -f Dockerfile.local .

./run_container.sh