#!/bin/bash

echo "Git update"
git pull origin lab2_meson

echo "CMake build"
rm -f build
meson setup build
meson compile -C build

echo "Execution"
./build/test_runner
