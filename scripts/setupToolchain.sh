#!/bin/bash

DOWNLOADER="curl -LO"
MD5SUM="md5sum"

BINUTILS_BASE_URL="https://ftp.gnu.org/gnu/binutils"
BINUTILS_NAME="binutils-2.35"
BINUTILS_PKG="$BINUTILS_NAME.tar.gz"
BINUTILS_MD5="63c597bd52f978d964028b7c3213d22e"

GCC_BASE_URL="https://ftp.gnu.org/gnu/gcc/gcc-10.2.0"
GCC_NAME="gcc-10.2.0"
GCC_PKG="$GCC_NAME.tar.gz"
GCC_MD5="941a8674ea2eeb33f5c30ecf08124874"

#sudo apt-get install cmake nasm curl qemu-system binutils gcc g++ build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo hxtools autoconf automake-1.15 libtool

mkdir -p cross
cd cross

if [ -e $BINUTILS_PKG ]; then
    md5=$($MD5SUM $BINUTILS_PKG | head -c 32)
    if [ $md5 != $BINUTILS_MD5 ]; then
        echo "MD5 does not match, downloading binutils..."
        rm -f $BINUTILS_PKG
        $DOWNLOADER $BINUTILS_BASE_URL/$BINUTILS_PKG

        rm -rf $BINUTILS_NAME
        echo "Extracting binutils..."
        tar -xzf $BINUTILS_PKG
    else
        echo "Skipping binutils download."
    fi
else
    echo "Downloading binutils..."
    $DOWNLOADER $BINUTILS_BASE_URL/$BINUTILS_PKG

    echo "Extracting binutils..."
    tar -xzf $BINUTILS_PKG
fi

if [ -e $GCC_PKG ]; then
    md5=$($MD5SUM $GCC_PKG | head -c 32)
    if [ $md5 != $GCC_MD5 ]; then
        echo "MD5 does not match, downloading gcc..."
        rm -f $GCC_PKG
        $DOWNLOADER $GCC_BASE_URL/$GCC_PKG

        rm -rf $GCC_NAME
        echo "Extracting gcc..."
        tar -xzf $GCC_PKG
    else
        echo "Skipping gcc download."
    fi
else
    echo "Downloading gcc..."
    $DOWNLOADER $GCC_BASE_URL/$GCC_PKG

    echo "Extracting gcc..."
    tar -xzf $GCC_PKG
fi

CROSS_PATH=$(realpath ./)
SYSROOT_PATH=$(realpath ../sysroot)

export PREFIX="$CROSS_PATH/kern"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

mkdir -p build-binutils-kernel
cd build-binutils-kernel

../$BINUTILS_NAME/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

cd ..

mkdir -p build-gcc-kernel
cd build-gcc-kernel
../$GCC_NAME/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

cd ..

# Setup limine
scripts/toolchain/limine.sh

echo "Building debug table exporter..."
git clone https://github.com/patryk3211/MierOSDebugExporter.git --depth 1
cd MierOSDebugExporter

cmake -S . -B build
cmake --build build --target all

cp build/debug-exporter ..
cd ..

rm -rf MierOSDebugExporter

mkdir -p ../sysroot/dev

#cp -r host-binutils-conf/* binutils-2.35/
#cp -r host-gcc-conf/* gcc-10.2.0/
#
#cd binutils-2.35/ld
#automake-1.15
#cd ../..
#
#cd gcc-10.2.0/libstdc++-v3
#autoconf
#cd ../..
#
#export PREFIX="$CROSS_PATH/host"
#export PATH="$PREFIX/bin:$PATH"
#export TARGET=i686-mieros
#
#mkdir build-binutils-hosted
#cd build-binutils-hosted
#
#../binutils-2.35/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot=$SYSROOT_PATH --disable-werror
#make
#make install
#
#cd ..
#
#mkdir build-gcc-hosted
#cd build-gcc-hosted
#../gcc-10.2.0/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot=$SYSROOT_PATH --disable-werror
#make all-gcc all-target-libgcc
#make install-gcc install-target-libgcc
#
#cd ..

#rm -rf binutils-2.35
#rm -rf gcc-10.2.0
rm -rf build-binutils-kernel
rm -rf build-gcc-kernel
#rm -rf build-binutils-hosted
#rm -rf build-gcc-hosted
rm -f binutils-2.35.tar.gz
rm -f gcc-10.2.0.tar.gz
