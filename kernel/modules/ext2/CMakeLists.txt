set(SOURCES src/main.cpp src/mount.cpp src/inode.cpp src/cache.cpp src/data_storage.cpp src/files.cpp src/open_close.cpp src/rw.cpp src/map.cpp)

add_kernel_module(ext2 fs ${SOURCES})

target_include_directories(ext2.mod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

