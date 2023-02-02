set(SOURCES main.cpp pci.cpp)

add_kernel_module(pci bus ${SOURCES})
