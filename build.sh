#!/bin/bash

echo "Git update"
git pull origin main

echo "CMake build"
make clean
make

echo "Execution"
./hello
