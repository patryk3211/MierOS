#!/bin/bash

if [ "$2" = "" ]; then
    echo "You have to specify the output file name!"
    exit
fi

mkdir tar

for arg in $(echo $1 | tr ';' ' '); do
    cp $arg tar
done

cd tar
tar -cvf $2 *
cd ..

rm -rf tar
