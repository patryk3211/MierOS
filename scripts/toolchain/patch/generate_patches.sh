#!/bin/bash

##
## Usage:
## generate_patches.sh <target_directory> <source_directory> <patch_directory>
##  <target_directory> contains files that should be patched in the <source_directory>
##  <patch_directory> is where the generated patches fill be stored
##

if [ "$1" == "" -o "$2" == "" -o "$3" == "" ]; then
    echo "Please provide the necessary arguments"
    exit
fi

ESCAPED_SRC=$(echo $2 | sed -e 's/\//\\\//g')
ESCAPED_DST=$(echo $1 | sed -e 's/\//\\\//g')

for f in $(find $1 -type f -printf "%P\n"); do
    mkdir -p $(dirname $3/$f)
    diff -Nau $2/$f $1/$f | sed -E -e "s/-{3}\s+$ESCAPED_SRC/--- a/g" -e "s/\+{3}\s+$ESCAPED_DST/+++ b/g" | tee > $3/$f.patch
done

