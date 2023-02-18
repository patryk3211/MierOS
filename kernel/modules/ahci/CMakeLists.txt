set(SOURCES src/main.cpp src/ata.cpp src/devfs.cpp src/drive.cpp src/pci.cpp)

add_kernel_module(ahci block ${SOURCES})
target_include_directories(ahci.mod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_module_dependency(ahci pci.mod)

