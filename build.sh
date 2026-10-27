#!/bin/bash

echo "Git update"
git pull origin main

echo "CMake build"
rm -f hello hello.o
make

echo "Execution"
./hello
