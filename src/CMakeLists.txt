cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -O0 -ggdb")
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(cc-vis)

list(APPEND libs LLVM clang-cpp raylib)

add_executable(cc-vis main.cc)
target_link_libraries(cc-vis PRIVATE ${libs})
