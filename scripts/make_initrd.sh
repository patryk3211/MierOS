#!/bin/bash

##
## Usage:
## ./make_initrd.sh <modules> <output_file> <module_init> <module_alias>
## <modules> is a semicolon separated list of file paths
## <output_file> is a filename of the output file
## <module_init> is a path to the modules.init file to be stored on the initrd
## <module_alias> is a path to the modules.alias file to be stored on the initrd
##

if [ "$2" = "" ]; then
    echo "You have to specify the output file name!"
    exit
fi

mkdir -p tar/lib/modules
mkdir -p tar/dev
mkdir -p tar/sys
mkdir -p tar/etc

for arg in $(echo $1 | tr ';' ' '); do
    cp -v $arg tar/lib/modules
done

cp -v $3 tar/etc
cp -v $4 tar/etc

echo "Compressing tar image..."

cd tar
tar -cf $2 *
cd ..

echo "Initrd created succesfully"

rm -rf tar
