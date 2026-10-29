#!/bin/bash

echo "Git update"
git pull origin Ð´Ðlab2

echo "CMake build"
rm -f test_runner *.o
make

echo "Execution"
./test_runner
