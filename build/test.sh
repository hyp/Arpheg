#!/bin/bash

rm -rf build
mkdir build
cd build
cmake ../

make coretest
make componentstest
make datatexttest
echo "Running unittests"
echo "Running coretest"
./coretest
echo "Running componentstest"
./componentstest
echo "Running datatexttest"
./datatexttest

