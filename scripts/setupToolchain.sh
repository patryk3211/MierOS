#!/bin/bash

TOOLCHAIN_TYPE=$1

if [ "$TOOLCHAIN_TYPE" == "llvm" ]; then
    echo "Compiling the LLVM Toolchain"
elif [ "$TOOLCHAIN_TYPE" == "gnu" ]; then
    echo "Compiling the GNU Toolchain"
else
    echo "Please specify a valid toolchain (llvm/gnu)"
    exit
fi

if [ "$2" == "" ]; then
    ACTION=""
elif [ "$2" == "rebuild" ]; then
    ACTION="rebuild"
elif [ "$2" == "cleanbuild" ]; then
    ACTION="cleanbuild"
else
    echo "Unknown action specified"
    exit
fi

# Install system dependencies
#sudo apt-get install cmake nasm curl qemu-system binutils gcc g++ build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo hxtools autoconf automake-1.15 libtool
pip install lief

# First we build the kernel compiler
if [ "$TOOLCHAIN_TYPE" == "llvm" ]; then
    ## I think we can just use the host compiler
    # scripts/toolchain/build_llvm_toolchain_kernel.sh
    echo "Not building kernel compiler, use host compiler"
else
    scripts/toolchain/build_gcc_toolchain_kernel.sh $ACTION
fi

# Prepare headers for userspace compiler
scripts/toolchain/prepare_libc_headers.sh $ACTION

# Build the userspace compiler
if [ "$TOOLCHAIN_TYPE" == "llvm" ]; then
    scripts/toolchain/build_llvm_toolchain_userspace.sh $ACTION
else
    scripts/toolchain/build_gcc_toolchain_userspace.sh $ACTION
fi

# Setup limine
scripts/toolchain/limine.sh $ACTION

# Build debug symbol exporter
scripts/toolchain/debug_exporter.sh $ACTION

mkdir -p sysroot/dev
mkdir -p sysroot/sys

