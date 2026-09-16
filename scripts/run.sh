#!/bin/bash
#~~~~~~~~~~

apt -y install cmake clang-tidy ccache g++

pushd ~/sc || exit
cmake -DCMAKE_BUILD_TYPE=Release -B cmake-build-release -S .
cmake --build cmake-build-release -j 12
ctest --test-dir cmake-build-release --output-on-failure

popd || exit