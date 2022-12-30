set(SOURCES main.cpp ata.cpp devfs.cpp)

add_kernel_module(ahci ${SOURCES})
