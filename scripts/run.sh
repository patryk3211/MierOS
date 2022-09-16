#!/bin/bash

if [ "$1" = "build" ]; then
	echo "Starting build"

	cmake --build build --target all
	cmake --install build
	cmake --build build --target copy_sysroot
fi

echo "Starting QEMU"

qemu-system-x86_64 -s -m 128M \
			-smp 4 \
			-device VGA,vgamem_mb=64 \
			-device ich9-ahci,id=ahci \
			-drive file=build/hdd.img,id=hddisk,if=none,format=raw \
			-device ide-hd,drive=hddisk,bus=ahci.0 \
			-usb \
			-audiodev alsa,id=1 \
			-device sb16 \
			-serial stdio \
			-no-reboot \
			-no-shutdown \
			-d guest_errors \
			-S
