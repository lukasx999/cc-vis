#!/bin/sh
set -euxo pipefail

clang++ main.cc -lLLVM -lclang-cpp -std=c++23 -Wall -Wextra -lraylib
./a.out
