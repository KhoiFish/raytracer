#!/bin/sh
rm -rf BuildOutput/CMakeBuildOutput
mkdir -p BuildOutput/CMakeBuildOutput
cd BuildOutput/CMakeBuildOutput
cmake ../../
make
cd ../../