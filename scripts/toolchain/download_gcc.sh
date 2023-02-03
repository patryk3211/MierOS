#!/bin/bash

EXTRACTED_NAME=$1

GCC_NAME="gcc-12.2.0"
GCC_PKG="$GCC_NAME.tar.gz"
GCC_MD5="d7644b494246450468464ffc2c2b19c3"
GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/$GCC_PKG"

scripts/toolchain/download.sh $GCC_NAME $GCC_PKG $GCC_URL $GCC_MD5 gzip cross $EXTRACTED_NAME

