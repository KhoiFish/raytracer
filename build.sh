#!/bin/sh
rm -rf BuildOutput/CMakeBuildOutput
mkdir BuildOutput/CMakeBuildOutput
cd BuildOutput/CMakeBuildOutput
cmake ../../
make
cd ../../