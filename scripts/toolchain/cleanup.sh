#!/bin/bash

pushd cross

rm -rf llvm-src binutils-src gcc-src
rm -rf build-binutils-kernel build-gcc-kernel
rm -rf build-binutils-hosted build-gcc-hosted
rm -rf llvm-kernel-build llvm-hosted-build

popd # cross

