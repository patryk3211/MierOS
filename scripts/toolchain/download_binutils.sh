#!/bin/bash

EXTRACTED_NAME=$1

BINUTILS_NAME="binutils-2.39"
BINUTILS_PKG="$BINUTILS_NAME.tar.gz"
BINUTILS_MD5="ab6825df57514ec172331e988f55fc10"
BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/$BINUTILS_PKG"

scripts/toolchain/download.sh $BINUTILS_NAME $BINUTILS_PKG $BINUTILS_URL $BINUTILS_MD5 gzip cross $EXTRACTED_NAME

patch -N -d cross/$EXTRACTED_NAME -p1 < scripts/toolchain/patch/gnu/binutils.patch

