#!/bin/bash

pushd cross

echo "Building debug table exporter..."
if [ -d "MierOSDebugExporter" ]; then
    pushd MierOSDebugExporter
    git pull
else
    git clone https://github.com/patryk3211/MierOSDebugExporter.git --depth 1
    pushd MierOSDebugExporter
fi

cmake -S . -B build
cmake --build build --target all

cp build/debug-exporter ..

popd # MierOSDebugExporter
popd # cross

