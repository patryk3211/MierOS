#!/bin/bash

# Prepare headers
pushd userspace/libs/mlibc
if [ -e "build" ]; then
    rm -rf build
fi

# Prepare for a headers only build of mlibc
meson setup -Dlinux_kernel_headers=../../../kernel/abi/include -Dprefix=/usr -Dheaders_only=true --cross-file='../x86_64-mieros.txt' build
pushd build
# Install the headers in our sysroot
meson install --destdir ../../../../sysroot --only-changed
popd # build
# Clear for cmake to reconfigure meson for a full build
rm -rf build
# Go back to root
popd # userspace/libs/mlibc

