#!/bin/bash

# Check for build type argument
if [ "$1" = "linux-debug" ]; then
    cmake --build --preset linux-build-debug
    cmake --build out/build/default-linux-debug --target run
elif [ "$1" = "linux-release" ]; then
    cmake --build --preset linux-build-release
    cmake --build out/build/default-linux-release --target run
else
    echo "Usage: ./run.sh [linux-debug|linux-release]"
fi
