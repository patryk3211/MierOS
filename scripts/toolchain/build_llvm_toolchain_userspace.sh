#!/bin/bash

scripts/toolchain/build_llvm_tblgen.sh

mkdir -p cross

CROSS_PATH=$(realpath ./cross)
SYSROOT_PATH=$(realpath ./sysroot)

if [ ! -e "cross/host-llvm" -o "$1" == "rebuild" ]; then
    scripts/toolchain/download_llvm.sh llvm-src

    echo "Building LLVM..."
    pushd cross
    cmake -S llvm-src/llvm -B llvm-hosted-build -G Ninja \
        -DCROSS_PATH="$CROSS_PATH" \
        -DSYSROOT_PATH="$SYSROOT_PATH" \
        -C ../scripts/toolchain/cmake/llvm-hosted.cmake

    pushd llvm-hosted-build
    if ninja clang lld; then
        if ninja compiler-rt cxx cxxabi unwind; then
            ninja install-clang install-lld \
                  install-compiler-rt install-cxx install-cxxabi \
                  install-unwind install-clang-resource-headers \
                  install-llvm-ar install-llvm-strip
        fi
    fi
    popd # llvm-hosted-build

    popd # cross
fi
