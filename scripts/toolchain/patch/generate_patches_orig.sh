#!/bin/bash

##
## Usage:
## generate_patches.sh <patch_directory> <source_directory>
##  <source_directory> should contain files where a pair of <file1> and <file1>.orig
##                     get used to create a patch of name <file1>.patch
##  <patch_directory> is where the generated patches fill be stored
##

if [ "$1" == "" -o "$2" == "" ]; then
    echo "Please provide the necessary arguments"
    exit
fi

ESCAPED_SRC=$(echo $2 | sed -e 's/\//\\\//g' -e 's/\./\\\./g')

for f in $(find $2 -name "*.orig" -type f -printf "%P\n"); do
    ORIGINAL=$f
    CHANGED=$(echo $f | head -c -6)
    PATCH=$1/$CHANGED.patch

    ESCAPED_CHANGED=$(echo $CHANGED | sed -e 's/\//\\\//g' -e 's/\./\\\./g')

    mkdir -p $(dirname $PATCH)
    diff -Nau $2/$ORIGINAL $2/$CHANGED \
        | sed -E \
        -e "s/-{3}\s+(.*?)$ESCAPED_CHANGED\.orig/--- \1$ESCAPED_CHANGED/g" \
        -e "s/-{3}\s+$ESCAPED_SRC/--- a/g" \
        -e "s/\+{3}\s+$ESCAPED_SRC/+++ b/g" \
        | tee > $PATCH
done

