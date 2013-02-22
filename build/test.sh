#!/bin/bash

rm -rf build
mkdir build
cd build
cmake ../
make coretest
echo "Running unittests"
echo "Running coretest"
./coretest
