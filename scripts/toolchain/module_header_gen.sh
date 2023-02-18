#!/bin/bash

pushd cross

echo "Building module header generator..."
if [ -d "MierOSHeaderGen" ]; then
    pushd MierOSHeaderGen
    git pull
else
    git clone --recursive https://github.com/patryk3211/MierOS-ModuleHeaderGenerator.git --depth 1 MierOSHeaderGen
    pushd MierOSHeaderGen
fi

cmake -S . -B build
cmake --build build --target all

cp build/modhdrgen ..

popd # MierOSHeaderGen
popd # cross


