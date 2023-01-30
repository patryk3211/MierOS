#!/bin/bash

DOWNLOADER="curl -LO"
MD5SUM="md5sum"

BINUTILS_BASE_URL="https://ftp.gnu.org/gnu/binutils"
BINUTILS_NAME="binutils-2.39"
BINUTILS_PKG="$BINUTILS_NAME.tar.gz"
BINUTILS_MD5="ab6825df57514ec172331e988f55fc10"

GCC_BASE_URL="https://ftp.gnu.org/gnu/gcc/gcc-12.2.0"
GCC_NAME="gcc-12.2.0"
GCC_PKG="$GCC_NAME.tar.gz"
GCC_MD5="d7644b494246450468464ffc2c2b19c3"

#sudo apt-get install cmake nasm curl qemu-system binutils gcc g++ build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo hxtools autoconf automake-1.15 libtool
pip install lief

mkdir -p cross
cd cross

function get_binutils() {
    if [ ! -e $BINUTILS_NAME ]; then
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
    fi
}

function get_gcc() {
    if [ ! -e $GCC_NAME ]; then
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
    fi
}

CROSS_PATH=$(realpath ./)
SYSROOT_PATH=$(realpath ../sysroot)

export PREFIX="$CROSS_PATH/kern"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

## We don't need to cross compile llvm for now since we are targeting x86_64

# LLVM_VERSION="15.0.6"
# if [ ! -e "llvm-project-$LLVM_VERSION.src" ]; then
#     if [ ! -e "llvm-project-$LLVM_VERSION.src.tar.xz" ]; then
#         LLVM_DOWNLOAD="https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION/llvm-project-$LLVM_VERSION.src.tar.xz"
#         $DOWNLOADER $LLVM_DOWNLOAD
#     fi
#
#     echo "Extracting LLVM..."
#     tar -xJf llvm-project-$LLVM_VERSION.src.tar.xz
#     cp -rv ../userspace/clang-prepare/* llvm-project-$LLVM_VERSION.src/
# fi
#
# if [ ! -e "llvm-host-tlb-build" ]; then
#     cmake -S llvm-project-$LLVM_VERSION.src/llvm -B llvm-host-tlb-build \
#         -DLLVM_ENABLE_PROJECTS='clang;compiler-rt;lld;clang-tools-extra' \
#         -DCMAKE_INSTALL_PREFIX="$CROSS_PATH/tblgen" \
#         -DCMAKE_BUILD_TYPE=Release \
#         -G Ninja
# fi
# cd llvm-host-tlb-build
# ninja llvm-tblgen clang-tblgen
# cd ..

## Configure LLVM
#cmake -S llvm-project-$LLVM_VERSION.src -B llvm-kernel-build \
#    -DCMAKE_SYSTEM_NAME="$LLVM_TARGET" \
#    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
#    -DLLVM_TABLEGEN="$CROSS_PATH/llvm-host-tlb-build/bin/llvm-tblgen" \
#    -DCLANG_TABLEGEN="$CROSS_PATH/llvm-host-tlb-build/bin/clang-tblgen" \
#    -DLLVM_DEFAULT_TARGET_TRIPLE="$LLVM_TARGET" \
#    -DLLVM_TARGET_ARCH=x86_64 \
#    -DLLVM_TARGETS_TO_BUILD=x86_64 \
#    -DLLVM_ENABLE_PROJECTS="clang" \
#    -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
#    -DCMAKE_BUILD_TYPE=Release \
#    -G Ninja
#
#cd llvm-kernel-build
#cmake --build llvm-kernel-build

if [ ! -e "auto-things" ]; then
    # Get specific versions of AutoConf and AutoMake
    AUTOCONF="autoconf-2.69"
    AUTOMAKE="automake-1.15.1"

    $DOWNLOADER https://ftp.gnu.org/gnu/autoconf/$AUTOCONF.tar.gz
    tar -xzf $AUTOCONF.tar.gz
    mkdir -p build-autoconf
    cd build-autoconf

    ../$AUTOCONF/configure --prefix="$(realpath ../auto-things)"
    make
    make install
    cd ..

    $DOWNLOADER https://ftp.gnu.org/gnu/automake/$AUTOMAKE.tar.gz
    tar -xzf $AUTOMAKE.tar.gz
    mkdir -p build-automake
    cd build-automake

    ../$AUTOMAKE/configure --prefix="$(realpath ../auto-things)"
    make
    make install
    cd ..

    rm -rf $AUTOCONF.tar.gz $AUTOMAKE.tar.gz $AUTOCONF $AUTOMAKE
    rm -rf build-autoconf build-automake
fi

# Inject our autotools into path
PATH=$(realpath auto-things/bin):$PATH

# Prepare headers
cd ../userspace/libs/mlibc
if [ -e "build" ]; then
    rm -rf build
fi

# Prepare for a headers only build of mlibc
meson setup -Dprefix=/usr -Dheaders_only=true --cross-file='../x86_64-mieros.txt' build
cd build
# Install the headers in our sysroot
meson install --destdir ../../../../sysroot --only-changed
cd ..
# Clear for cmake to reconfigure meson for a full build
rm -rf build
# Go back to compiling gcc
cd ../../../cross

export PREFIX="$CROSS_PATH/host"
export TARGET=x86_64-mieros
export PATH="$PREFIX/bin:$PATH"

# -DCMAKE_TOOLCHAIN_FILE="$CROSS_PATH/../scripts/x86_64-hosted_compiler_toolchain.cmake" \
# -DCMAKE_SYSTEM_NAME="Generic" \

# LLVM_TARGET=x86_64-pc-mieros-elf
# cmake -S llvm-project-$LLVM_VERSION.src/llvm -B llvm-hosted-build \
#     -DCMAKE_INSTALL_PREFIX="$PREFIX" \
#     -DLLVM_TABLEGEN="$CROSS_PATH/llvm-host-tlb-build/bin/llvm-tblgen" \
#     -DCLANG_TABLEGEN="$CROSS_PATH/llvm-host-tlb-build/bin/clang-tblgen" \
#     -DLLVM_DEFAULT_TARGET_TRIPLE="$LLVM_TARGET" \
#     -DBUILD_SHARED_LIBS=false \
#     -DLLVM_TARGET_ARCH="x86_64" \
#     -DLLVM_TARGETS_TO_BUILD="X86" \
#     -DLLVM_ENABLE_PROJECTS="clang" \
#     -DLLVM_ENABLE_RUNTIMES="compiler-rt;libcxx;libcxxabi" \
#     -DLLVM_ENABLE_THREADS=OFF \
#     -DDEFAULT_SYSROOT="$SYSROOT_PATH" \
#     -DCMAKE_BUILD_TYPE=Release \
#     -G Ninja
#
# cmake --build llvm-hosted-build

if [ -e "$PREFIX/bin/$TARGET-ld" ]; then
    echo "Hosted binutils already built, skipping..."
else
    get_binutils

    cp -vr ../userspace/gcc-prepare/binutils/* $BINUTILS_NAME/
    cd $BINUTILS_NAME/ld
    automake
    cd ../..

    mkdir -p build-binutils-hosted
    cd build-binutils-hosted

    ../$BINUTILS_NAME/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot=$SYSROOT_PATH --disable-werror --disable-nls
    make -j4
    make install

    cd ..
fi

if [ -e "$PREFIX/bin/$TARGET-gcc" ]; then
    echo "Hosted gcc already built, skipping..."
else
    get_gcc

    cp -vr ../userspace/gcc-prepare/gcc/* $GCC_NAME/
    cd $GCC_NAME/libstdc++-v3
    autoconf
    cd ../..

    mkdir -p build-gcc-hosted
    cd build-gcc-hosted
    ../$GCC_NAME/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot=$SYSROOT_PATH --enable-languages=c,c++ --disable-werror --disable-nls
    make all-gcc -j4
    make all-target-libgcc -j4
    make install-gcc
    make install-target-libgcc

    # TODO: Build libstdc++

    cd ..
fi

#rm -rf $BINUTILS_NAME
#rm -rf $GCC_NAME
#rm -rf build-binutils-hosted
#rm -rf build-gcc-hosted
rm -f $BINUTILS_PKG
rm -f $GCC_PKG

cd ..

# Setup limine
scripts/toolchain/limine.sh

# Build debug symbol exporter
scripts/toolchain/debug_exporter.sh

mkdir -p sysroot/dev
