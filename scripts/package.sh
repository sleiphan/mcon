#!/usr/bin/env sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build
cpack --config build/CPackConfig.cmake -G DEB
