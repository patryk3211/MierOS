#!/bin/bash

if [ "$1" == "cleanbuild" ]; then
    rm -rf cross/tblgen
fi

if [ ! -e "cross/llvm-host-tbl-build/bin/llvm-tblgen" -o ! -e "cross/llvm-host-tbl-build/bin/clang-tblgen" ]; then
    scripts/toolchain/download_llvm.sh llvm-src

    echo "Building LLVM TableGen Tools"
    pushd cross
    cmake -S llvm-src/llvm -B llvm-host-tbl-build \
        -DLLVM_ENABLE_PROJECTS='clang;compiler-rt;lld;clang-tools-extra' \
        -DCMAKE_BUILD_TYPE=Release \
        -G Ninja

    pushd llvm-host-tbl-build
    ninja llvm-tblgen clang-tblgen

    popd # llvm-host-tlb-build
    popd # cross
fi

echo "Build complete"

