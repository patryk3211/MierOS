set(SOURCES main.cpp mount.cpp inode.cpp cache.cpp data_storage.cpp files.cpp open_close.cpp rw.cpp map.cpp)

add_kernel_module(ext2 ${SOURCES})
