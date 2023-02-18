set(SOURCES src/main.cpp src/pci.cpp)

add_kernel_module(pci bus ${SOURCES})
target_include_directories(pci.mod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

