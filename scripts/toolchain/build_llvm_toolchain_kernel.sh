#!/bin/bash

scripts/toolchain/build_llvm_tblgen.sh

if [ ! -e "cross/kern-llvm" ]; then
    scripts/toolchain/download_llvm.sh llvm-src

    echo "Building LLVM..."
    pushd cross
    cmake -S llvm-src -B llvm-kernel-build -G Ninja \
        -DCROSS_PATH=$(realpath .) \
        -C ../scripts/toolchain/cmake/llvm-kernel.cmake

    pushd llvm-kernel-build
    ninja clang lld
    ninja compiler-rt

    ninja install-clang install-lld install-compiler-rt install-clang-resource-headers
    popd # llvm-kernel-build

    popd # cross
fi

