add_executable(ext2.mod main.cpp)

install(TARGETS ext2.mod DESTINATION ${SYSROOT_DIR}/lib/modules)
