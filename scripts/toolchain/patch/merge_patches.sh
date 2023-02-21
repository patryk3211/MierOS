#!/bin/bash

##
## Usage
## merge_patches.sh <directory_of_patches> <output_patch_name>
##

if [ "$1" == "" -o "$2" == "" ]; then
    echo "Please specify the necessary arguments"
    exit
fi

cat $(find $1 -type f) > $2

