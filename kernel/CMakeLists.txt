set(CMAKE_C_FLAGS "-g -fvisibility=default -nostdlib -ffreestanding -Wall -Wextra -mcmodel=large -fPIC -Wl,-export-dynamic -mno-red-zone")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fno-use-cxa-atexit")

set(CMAKE_CXX_COMPILER "${CROSS_DIR}/kern/bin/${ARCH}-elf-g++")
set(CMAKE_C_COMPILER "${CROSS_DIR}/kern/bin/${ARCH}-elf-gcc")

set(KERNEL_SOURCES src/init.cpp
                   src/dmesg.c
                   src/trace.c
                   src/stdlib.c
                   src/initrd.cpp
                   src/memory/physical.cpp
                   src/memory/liballoc.c
                   src/memory/liballoc_plugs.cpp
                   src/memory/virtual.cpp
                   src/memory/ppage.cpp
                   src/memory/kbuffer.cpp
                   src/tasking/thread.cpp
                   src/tasking/scheduler.cpp
                   src/tasking/process.cpp
                   src/modules/module_manager.cpp
                   src/modules/module.cpp
                   src/fs/filesystem.cpp
                   src/fs/modulefs.cpp
                   src/fs/devicefs.cpp
                   src/fs/vnode.cpp
                   src/util/crc.c
                   src/streams/stream.cpp
                   src/streams/filestream.cpp)

set(TESTS_SOURCES src/tests/test.cpp
                  src/tests/memory.cpp
                  src/tests/stdlib.cpp
                  src/tests/atomic.cpp)

file(GLOB_RECURSE ARCH_SOURCES CONFIGURE_DEPENDS src/arch/${ARCH}/*.cpp src/arch/${ARCH}/*.c src/arch/${ARCH}/*.s)

if(${TESTING})
    set(KERNEL_SOURCES ${KERNEL_SOURCES} ${TESTS_SOURCES})
endif()

add_executable(kernel.bin ${KERNEL_SOURCES} ${ARCH_SOURCES})

set_target_properties(kernel.bin PROPERTIES
                      PREFIX ""
                      SUFFIX "")

add_custom_command(TARGET kernel.bin PRE_BUILD 
                    COMMAND nasm ARGS -o ap_starter ${CMAKE_CURRENT_SOURCE_DIR}/ap_starter.s -f bin
                    COMMAND ld ARGS -r -b binary -o ap_starter.o ap_starter
                    VERBATIM
                    BYPRODUCTS ap_starter ap_starter.o
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Assembling Application Core starter...")

add_custom_command(TARGET kernel.bin POST_BUILD
                    COMMAND elfedit ARGS --output-type DYN kernel.bin
                    VERBATIM
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(TARGET kernel.bin POST_BUILD
                    COMMAND ${CROSS_DIR}/debug-exporter kernel.bin kernel.map
                    VERBATIM
                    BYPRODUCTS kernel.map
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Exporting debug line symbols...")

add_compile_definitions(${ARCH}=1)

target_link_options(kernel.bin PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -pie -Wl,-export-dynamic -z max-page-size=0x1000 ${CMAKE_CURRENT_BINARY_DIR}/ap_starter.o -Wl,--no-dynamic-linker)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/std/include)

install(TARGETS kernel.bin DESTINATION ${SYSROOT_DIR}/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kernel.map DESTINATION ${SYSROOT_DIR}/)

add_subdirectory(modules)
