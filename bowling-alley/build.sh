#!/bin/bash
# compile the renderer host (the Metal kernel is compiled at run time from src/render.metal)
set -e
cd "$(dirname "$0")"
clang++ -std=c++17 -O2 -fobjc-arc -Wno-unused-function -Wno-unused-variable src/main.mm -o dronecam -framework Metal -framework Foundation -lz
