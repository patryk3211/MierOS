add_executable(pci.mod main.cpp pci.cpp)

install(TARGETS pci.mod DESTINATION ${SYSROOT_DIR}/lib/modules)
