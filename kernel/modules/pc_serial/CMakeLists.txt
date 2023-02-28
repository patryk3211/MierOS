add_kernel_module(pc_serial tty src/main.cpp src/devfs.cpp)

target_include_directories(pc_serial.mod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

