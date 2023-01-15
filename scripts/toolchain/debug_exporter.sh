#!/bin/bash

cd cross

echo "Building debug table exporter..."
if [ -d "MierOSDebugExporter" ]; then
    cd MierOSDebugExporter
    git pull
else
    git clone https://github.com/patryk3211/MierOSDebugExporter.git --depth 1
    cd MierOSDebugExporter
fi

cmake -S . -B build
cmake --build build --target all

cp build/debug-exporter ..
cd ../..
