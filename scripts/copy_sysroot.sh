#!/bin/bash
LOOP_DEV=$(udisksctl loop-setup --file hdd.img | grep -o '/[^ ]*' | head -c -2)
MNT=$(udisksctl mount --block-device $(echo $LOOP_DEV)p1 | grep -o '/media[^ ]*' | head -c -2)

cp -r -u $1/* $MNT/

udisksctl unmount --block-device $(echo $LOOP_DEV)p1
udisksctl loop-delete --block-device $LOOP_DEV
