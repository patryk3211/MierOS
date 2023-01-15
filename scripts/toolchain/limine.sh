#!/bin/bash

echo "Installing limine..."
if [ -d "limine" ]; then
    cd limine
    git pull
    cd ..
else
    git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth 1
fi
cd limine

make all
cd ..

mkdir -p sysroot/boot
cp -v limine/limine.sys sysroot/boot/
