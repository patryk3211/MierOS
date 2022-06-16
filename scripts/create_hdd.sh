#!/bin/bash

if [ "$1" = "" ]; then
    echo "You have to specify the output file name!"
    exit
fi

DIR=$(dirname $1)
if [ "$DIR" != "" ]; then
    mkdir -p $DIR
fi

mkdir -p $(dirname $1)

dd if=/dev/zero of=$1 count=262144

mkdir -p imgmnt
# "g\nn\n\n\n10240\nt\n4\nn\n\n\n\nw\n"

printf "g\nn\n\n\n\nx\nu\n1cf088df-2a85-c640-871f-e7cc1ac6ba3b\nr\nw\n" | sudo fdisk $1
sudo losetup -P /dev/loop100 $1
sudo mkfs.ext2 /dev/loop100p1 -E root_owner=$UID:$GID
sudo mount /dev/loop100p1 imgmnt

#sudo grub-install --force --target=i386-pc --root-directory=imgmnt --boot-directory=imgmnt/boot --grub-mkdevicemap=sysroot/boot/grub/device.map --no-floppy --modules="biosdisk part_msdos ext2 configfile normal multiboot" /dev/loop100
limine/limine-deploy $1

GROUP_NAME=$(id -gn)

sudo chown -hR $USER:$GROUP_NAME imgmnt/*
sudo chown -hR $USER:$GROUP_NAME imgmnt

sudo umount imgmnt
sudo losetup -d /dev/loop100

rmdir imgmnt
