add_executable(ext2.mod main.cpp mount.cpp inode.cpp cache.cpp data_storage.cpp files.cpp open_close.cpp rw.cpp map.cpp)

install(TARGETS ext2.mod DESTINATION ${SYSROOT_DIR}/lib/modules)
