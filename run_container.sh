#!/bin/bash

#docker run -it -v $(pwd)/:/app/kylea kylea ./build_test.sh
docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -it -v $(pwd)/:/opt/app/kylea kylea