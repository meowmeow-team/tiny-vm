#!/usr/bin/env bash
set -e

rm -rf build/
mkdir build
cd build

cmake .. -DBUILD_TESTING=OFF
cmake --build .

./tiny-vm
