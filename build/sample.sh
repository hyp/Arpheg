#!/bin/bash

if [ $# -ne 1 ]
then
	echo "Expected the sample's name as an argument!"
	return
fi

rm -rf build
mkdir build
cd build
cmake ../

make $1
echo "Running $1"
./${1}
