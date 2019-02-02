#!/bin/sh
rm -rf CMakeBuildOutput
mkdir CMakeBuildOutput
cd CMakeBuildOutput
cmake ../
make
cd ..