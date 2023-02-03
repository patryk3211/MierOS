#!/bin/bash

mkdir -p cross/build-binutils-kernel
CROSS_PATH=$(realpath ./cross)
SYSROOT_PATH=$(realpath ./sysroot)

export PREFIX="$CROSS_PATH/kern-gcc"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

CORE_COUNT=$(nproc)
ACTION=$1

if [ "$ACTION" == "cleanbuild" ]; then
    rm -rf $PREFIX
fi

if [ ! -e "$PREFIX/bin/$TARGET-ld" ]; then
    scripts/toolchain/download_binutils.sh binutils-src

    pushd cross/build-binutils-kernel

    echo "Building binutils..."
    ../binutils-src/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror

    make -j$CORE_COUNT
    make install

    popd # cross/build-binutils-kernel
fi

if [ ! -e $PREFIX/bin/$TARGET-gcc ]; then
    scripts/toolchain/download_gcc.sh gcc-src

    mkdir -p cross/build-gcc-kernel
    pushd cross/build-gcc-kernel

    echo "Building gcc..."
    ../gcc-src/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

    make all-gcc -j$CORE_COUNT
    make all-target-libgcc -j$CORE_COUNT
    make install-gcc install-target-libgcc

    popd # cross/build-gcc-kernel
fi

echo "Build complete"

