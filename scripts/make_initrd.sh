#!/bin/bash

##
## Usage:
## ./make_initrd.sh <mappings> <output_file>
## <mappings> is a semicolon separated list of <key>=<value> mappings where <key> is
##            the input file and <value> is it's location on the initrd
## <output_file> is the filename of the output file
##

if [ "$1" = "" ]; then
    echo "You have to specify file mappings!"
    exit
fi

if [ "$2" = "" ]; then
    echo "You have to specify the output file name!"
    exit
fi

TAR_ROOT=$(mktemp -d)
if [ "$TAR_ROOT" = "" ]; then
    echo "Failed to create a temporary directory!"
    exit
fi

echo "Moving files into temporary directory..."

for map in $(echo $1 | tr ';' ' '); do
    array=( $(echo $map | tr '=' ' ') )
    printf "'%s' -> '%s'\n" ${array[0]} ${array[1]}

    mkdir -p $(dirname $TAR_ROOT/${array[1]})
    cp ${array[0]} $TAR_ROOT/${array[1]}
done

mkdir -p $TAR_ROOT/dev
mkdir -p $TAR_ROOT/sys

echo "Compressing tar image..."
RETURN_TO=$(pwd)
FILE=$(realpath $2)

cd $TAR_ROOT
if tar -cf $FILE *; then
    echo "Initrd created successfully"
else
    echo "Failed to create initrd"
fi
cd $RETURN_TO

rm -rf $TAR_ROOT

