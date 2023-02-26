#!/bin/bash

scripts/toolchain/build_llvm_tblgen.sh

mkdir -p cross

CROSS_PATH=$(realpath ./cross)
SYSROOT_PATH=$(realpath ./sysroot)

if [ ! -e "cross/host-llvm" -o "$1" == "rebuild" ]; then
    scripts/toolchain/download_llvm.sh llvm-src

    CMAKE_FRESH=""
    if [ "$1" == "rebuild" ]; then
        CMAKE_FRESH="--fresh"
    fi

    echo "Building LLVM..."
    pushd cross
    cmake -S llvm-src/llvm -B llvm-hosted-build -G Ninja \
        -DCROSS_PATH="$CROSS_PATH" \
        -DSYSROOT_PATH="$SYSROOT_PATH" \
        -C ../scripts/toolchain/cmake/llvm-hosted.cmake $CMAKE_FRESH

    pushd llvm-hosted-build
    if ninja clang lld; then
        if ninja compiler-rt unwind cxx cxxabi; then
            ninja install-clang install-lld \
                  install-compiler-rt install-cxx install-cxxabi \
                  install-unwind install-clang-resource-headers \
                  install-llvm-ar install-llvm-strip
        fi
    fi
    popd # llvm-hosted-build

    cp -v host-llvm/lib/x86_64-pc-mieros-elf/*.so.* ../sysroot/lib

    popd # cross
fi
