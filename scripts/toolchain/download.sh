#!/bin/bash

# Name the arguments
NAME=$1
PKG=$2
SOURCE=$3
MD5=$4

COMPRESSION_TYPE=""
case $5 in
    gzip)
        COMPRESSION_TYPE="-z"
        ;;
    xz)
        COMPRESSION_TYPE="-J"
        ;;
    *)
        echo "Unknown compression type provided"
        exit
esac

DEST=$7
function extract {
    rm -rf $NAME
    echo "Extracting $NAME..."
    tar $COMPRESSION_TYPE -xf $PKG

    mv $NAME $DEST
    rm $PKG
}

function download_and_extract {
    echo "Downloading $PKG..."
    curl -L $SOURCE -o $PKG
    
    extract
}

mkdir -p $6
pushd $6
if [ ! -e $DEST ]; then
    if [ -e $PKG ]; then
        if [ "$MD5" != "none" -a "$(md5sum $PKG | head -c 32)" != "$MD5" ]; then
            echo "MD5 does not match, downloading $NAME..."
            rm -f $PKG
            
            download_and_extract
        else
            echo "Skipping $NAME download."

            extract
        fi
    else
        download_and_extract
    fi
fi
popd

