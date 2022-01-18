set(CMAKE_CXX_FLAGS "-g -nostdlib -ffreestanding -Wall -Wextra -fno-exceptions -fno-rtti -mcmodel=kernel -fno-use-cxa-atexit")
set(CMAKE_C_FLAGS "-g -nostdlib -ffreestanding -Wall -Wextra -mcmodel=kernel -fno-use-cxa-atexit")

set(CMAKE_CXX_COMPILER "${CROSS_DIR}/kern/bin/${ARCH}-elf-g++")
set(CMAKE_C_COMPILER "${CROSS_DIR}/kern/bin/${ARCH}-elf-gcc")

set(KERNEL_SOURCES src/init.cpp
                   src/dmesg.c
                   src/stdlib.c
                   src/memory/physical.cpp
                   src/memory/liballoc.c
                   src/memory/liballoc_plugs.cpp
                   src/memory/virtual.cpp
                   src/memory/ppage.cpp)

set(TESTS_SOURCES src/tests/test.cpp
                  src/tests/memory.cpp
                  src/tests/stdlib.cpp)

file(GLOB_RECURSE ARCH_SOURCES CONFIGURE_DEPENDS src/arch/${ARCH}/*.cpp src/arch/${ARCH}/*.c src/arch/${ARCH}/*.s)

if(${TESTING})
    set(KERNEL_SOURCES ${KERNEL_SOURCES} ${TESTS_SOURCES})
endif()

add_executable(kernel.bin ${KERNEL_SOURCES} ${ARCH_SOURCES})

add_compile_definitions(${ARCH}=1)

target_link_options(kernel.bin PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -z max-page-size=0x1000)
target_include_directories(kernel.bin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(TARGETS kernel.bin DESTINATION ${SYSROOT_DIR}/)
