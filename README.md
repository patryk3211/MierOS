# MierOS

## About
MierOS is a hobby operating system that I develop in my free time and when I'm bored.

My goal is to develop a system which can run simple graphical games like Solitaire and perhaps more complex stuff further down the road.

## Features
Currently there are not many features implemented, but some of the bigger ones are:
- AHCI driver
- Ext2 filesystem (read only)
- Simple task scheduler

## Building
1. Building the toolchain

    In order to build the toolchain you will need to run `scripts/setupToolchain.sh`. This script will download all the required packages, build the GNU C/C++ compiler and Binutils, Limine bootloader and a utility program to export kernel symbols for debugging.

2. Creating the disk image

    In order to create a disk image you need to run `scripts/create_hdd.sh build/hdd.img`. This script will create a raw hard drive image that will be used for storing and booting the OS. It automatically formats the drive and creates a partition with a fixed UUID to make development easier.

3. Building the kernel

    To build the kernel you first need to run `cmake -S . -B build` to generate the cmake directory. After that run `cmake --build build --target kernel.bin` to build the kernel, this is only required when you first start developing, later cmake should automatically handle the dependencies correctly. After building the kernel you can run `cmake --build build` to build all the modules and other things.

4. Running the OS

    There are 2 ways to launch the system. You can use Visual Studio Code launch target or you can run `scripts/run.sh`. If you are not using GDB you can remove the `-S` argument to not stop QEMU at launch.
