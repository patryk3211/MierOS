#!/bin/bash

LLVM_VERSION="15.0.6"

EXTRACTED_NAME=$1
LLVM_NAME="llvm-project-$LLVM_VERSION.src"
LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION/$LLVM_NAME.tar.xz"

scripts/toolchain/download.sh $LLVM_NAME $LLVM_NAME.tar.xz $LLVM_URL none xz cross $EXTRACTED_NAME

# Patch llvm with our changes
echo "Patching LLVM..."
for p in $(find scripts/toolchain/patch/llvm/ -type f); do
    patch -b -z '.orig' -N -d cross/$EXTRACTED_NAME -p1 < $p
done

