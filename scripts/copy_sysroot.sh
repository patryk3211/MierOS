#!/bin/bash
mkdir imgmnt

cat $2 | sudo -S losetup -P /dev/loop100 hdd.img
cat $2 | sudo -S mount /dev/loop100p1 imgmnt

#LOOP_DEV=$(udisksctl loop-setup --file hdd.img | grep -o '/[^ ]*' | head -c -2)
#MNT=$(udisksctl mount --block-device $(echo $LOOP_DEV)p1 | grep -o '/media[^ ]*' | head -c -2)

cp -r -u $1/* imgmnt/

cat $2 | sudo -S umount imgmnt
cat $2 | sudo -S losetup -d /dev/loop100

#udisksctl unmount --block-device $(echo $LOOP_DEV)p1
#udisksctl loop-delete --block-device $LOOP_DEV

rmdir imgmnt
