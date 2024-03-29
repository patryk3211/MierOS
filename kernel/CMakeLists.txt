set(CMAKE_ASM_FLAGS "-Wa,-msyntax=intel -Wa,-mnaked-reg")

set(KERNEL_C_FLAGS "-g -fvisibility=default -nostdlib -ffreestanding -fno-builtin -Wall -Wextra -mcmodel=kernel -fPIC -mno-red-zone -mno-sse")
set(KERNEL_CXX_FLAGS "-fno-exceptions -fno-rtti -fno-use-cxa-atexit")

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang")
set(KERNEL_C_FLAGS "${KERNEL_C_FLAGS} --target=x86_64-pc-none-elf -fno-plt")
set(KERNEL_CXX_FLAGS "${KERNEL_CXX_FLAGS}")

set(CMAKE_C_FLAGS "${KERNEL_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${KERNEL_CXX_FLAGS} -fvisibility-inlines-hidden")

set(KERNEL_SOURCES src/init.cpp
                   src/dmesg.c
                   src/trace.c
                   src/stdlib.c
                   src/printf.c
                   src/memory/physical.cpp
                   src/memory/liballoc.c
                   src/memory/liballoc_plugs.cpp
                   src/memory/virtual.cpp
                   src/memory/ppage.cpp
                   src/memory/kbuffer.cpp
                   src/tasking/thread.cpp
                   src/tasking/scheduler.cpp
                   src/tasking/process.cpp
                   src/tasking/sleep_queue.cpp
                   src/tasking/futex.cpp
                   src/locking/recursive_mutex.cpp
                   src/locking/semaphore.cpp
                   src/modules/module_manager.cpp
                   src/modules/module.cpp
                   src/fs/filesystem.cpp
                   src/fs/modulefs.cpp
                   src/fs/devicefs.cpp
                   src/fs/vnode.cpp
                   src/fs/vfs.cpp
                   src/fs/initrdfs.cpp
                   src/fs/systemfs.cpp
                   src/fs/vnodefs.cpp
                   src/util/crc.c
                   src/util/uuid.cpp
                   src/streams/stream.cpp
                   src/streams/filestream.cpp
                   src/streams/termios.cpp
                   src/streams/streamwrapper.cpp
                   src/streams/directorystream.cpp
                   src/streams/dmesgstream.cpp
                   src/streams/pipestream.cpp
                   src/memory/page/unresolvedpage.cpp
                   src/memory/page/anonpage.cpp
                   src/memory/page/filepage.cpp
                   src/memory/page/memoryfilepage.cpp
                   src/event/event_manager.cpp
                   src/event/uevent.cpp)

set(TESTS_SOURCES src/tests/test.cpp
                  src/tests/memory.cpp
                  src/tests/stdlib.cpp
                  src/tests/atomic.cpp)

file(GLOB_RECURSE ARCH_SOURCES CONFIGURE_DEPENDS src/arch/${ARCH}/*.cpp src/arch/${ARCH}/*.c src/arch/${ARCH}/*.s)

file(GLOB_RECURSE SYSCALL_SOURCES CONFIGURE_DEPENDS src/tasking/syscalls/*.cpp)
set(KERNEL_SOURCES ${KERNEL_SOURCES} ${SYSCALL_SOURCES})

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
                    COMMAND ${CMAKE_SOURCE_DIR}/scripts/transform_kernel.py ARGS kernel.bin
                    VERBATIM
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Changing ELF type of kernel.bin")

add_custom_command(TARGET kernel.bin POST_BUILD
                    COMMAND ${CROSS_DIR}/debug-exporter kernel.bin kernel.map
                    VERBATIM
                    BYPRODUCTS kernel.map
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Exporting debug line symbols...")

add_compile_definitions(${ARCH}=1)

if(${DEBUG})
    add_compile_definitions(DEBUG=true)
endif()

target_link_options(kernel.bin PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -z max-page-size=0x1000 -pie -Wl,-export-dynamic ${CMAKE_CURRENT_BINARY_DIR}/ap_starter.o -Wl,--no-dynamic-linker)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/std/include ${CMAKE_CURRENT_SOURCE_DIR}/abi/include)

install(TARGETS kernel.bin DESTINATION ${SYSROOT_DIR}/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kernel.map DESTINATION ${SYSROOT_DIR}/)

# Install kernel headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/abi/include/asm DESTINATION ${SYSROOT_DIR}/usr/include)

add_subdirectory(modules)

file(GLOB_RECURSE FORMAT_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c
                                                   include/*.hpp include/*.h
                                                   modules/*.cpp modules/*.c
                                                   modules/*.hpp modules/*.h
                                                   std/*.hpp std/*.h)
add_custom_target(format_kernel_code sh ${CMAKE_SOURCE_DIR}/scripts/format_code.sh "${FORMAT_SOURCES}" COMMENT "Formatting code..." VERBATIM)
