cmake_minimum_required(VERSION 3.10)

project(MierOS VERSION 0.0.1)

enable_language(C)
enable_language(CXX)
enable_language(ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(ARCH "x86_64" CACHE STRING "Set the kernel's target architecture")
option(TESTING "Enable kernel and module testing" ON)

set(CROSS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cross")
set(SYSROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sysroot")

add_subdirectory(kernel)

file(COPY ${CMAKE_SOURCE_DIR}/scripts/create_hdd.sh DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/scripts/copy_sysroot.sh DESTINATION ${CMAKE_BINARY_DIR})

add_custom_target(copy_sysroot "sh" "copy_sysroot.sh" "${SYSROOT_DIR}"
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
