#!/bin/sh
set -euxo pipefail

clang++ main.cc -lLLVM -lclang -lclang-cpp -std=c++23 -Wall -Wextra
./a.out
