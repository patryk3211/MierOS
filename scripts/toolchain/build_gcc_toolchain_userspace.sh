#!/bin/bash

CROSS_PATH=$(realpath ./cross)
SYSROOT_PATH=$(realpath ./sysroot)

export PREFIX="$CROSS_PATH/host-gcc"
export TARGET=x86_64-mieros
export PATH="$PREFIX/bin:$PATH"

CORE_COUNT=$(nproc)
ACTION=$1

if [ "$ACTION" == "cleanbuild" ]; then
    rm -rf $PREFIX
    rm -rf cross/autotools
fi

if [ ! -e "autotools" ]; then
    AUTOCONF="autoconf-2.69"
    AUTOMAKE="automake-1.15.1"

    scripts/toolchain/download.sh $AUTOCONF $AUTOCONF.tar.gz "https://ftp.gnu.org/gnu/autoconf/$AUTOCONF.tar.gz" none gzip cross autoconf-src
    scripts/toolchain/download.sh $AUTOMAKE $AUTOMAKE.tar.gz "https://ftp.gnu.org/gnu/automake/$AUTOMAKE.tar.gz" none gzip cross automake-src

    mkdir -p cross/autoconf-build
    mkdir -p cross/automake-build

    pushd cross
    pushd autoconf-build

    ../autoconf-src/configure --prefix=$(realpath ../autotools)
    make -j$CORE_COUNT
    make install

    popd # autoconf-build

    pushd automake-build
    
    ../automake-src/configure --prefix=$(realpath ../autotools)
    make -j$CORE_COUNT
    make install

    popd # automake-build

    # Cleanup
    rm -rf autoconf-src automake-src autoconf-build automake-build

    popd # cross
fi

PATH=$(realpath autotools/bin):$PATH

if [ -e "$PREFIX/bin/$TARGET-ld" ]; then
    echo "Hosted binutils already built, skipping..."
else
    scripts/toolchain/download_binutils.sh binutils-src

    echo "Building binutils..."
    cp -vr scripts/toolchain/patch/gcc/binutils/* cross/binutils-src/
    pushd cross/binutils-src/ld
    automake
    popd # cross/binutils-src/ld

    mkdir -p cross/build-binutils-hosted
    pushd cross/build-binutils-hosted

    ../binutils-src/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot=$SYSROOT_PATH --disable-werror --disable-nls

    make -j$CORE_COUNT
    make install

    popd # cross/build-binutils-hosted
fi

if [ -e "$PREFIX/bin/$TARGET-gcc" ]; then
    echo "Hosted gcc already built, skipping..."
else
    scripts/toolchain/download_gcc.sh gcc-src

    echo "Building gcc..."
    cp -vr scripts/toolchain/patch/gcc/gcc/* cross/gcc-src/
    pushd cross/gcc-src/libstdc++-v3
    autoconf
    popd # cross/gcc-src/libstdc++-v3

    mkdir -p cross/build-gcc-hosted
    pushd cross/build-gcc-hosted

    ../gcc-src/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot=$SYSROOT_PATH --enable-languages=c,c++ --disable-werror --disable-nls
    make all-gcc -j$CORE_COUNT
    make all-target-libgcc -j$CORE_COUNT
    make install-gcc
    make install-target-libgcc

    # TODO: Build libstdc++

    popd # cross/build-gcc-hosted
fi

echo "Build complete"

