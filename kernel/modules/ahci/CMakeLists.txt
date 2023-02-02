set(SOURCES main.cpp ata.cpp devfs.cpp drive.cpp pci.cpp)

add_kernel_module(ahci block ${SOURCES})
