add_executable(ahci.mod main.cpp ata.cpp devfs.cpp)

install(TARGETS ahci.mod DESTINATION ${SYSROOT_DIR}/lib/modules)
