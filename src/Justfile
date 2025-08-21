configure:
    cmake -B build -GNinja -DCMAKE_CXX_COMPILER=clang++

build: configure
    cmake --build build

run: build
    ./build/cc-vis

dump:
    clang++ -Xclang -ast-dump -fsyntax-only foo.cc

