set(CMAKE_CXX_COMPILER "${CROSS_DIR}/host/bin/${ARCH}-elf-g++")
set(CMAKE_C_COMPILER "${CROSS_DIR}/host/bin/${ARCH}-elf-gcc")

#set(ENV{CC} ${CMAKE_C_COMPILER})
#set(ENV{CXX} ${CMAKE_CXX_COMPILER})
#
#set(ENV{CC_LD} ${CMAKE_C_COMPILER})
#set(ENV{CXX_LD} ${CMAKE_CXX_COMPILER})

execute_process(COMMAND meson "-Dprefix=/usr" build WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mlibc)

add_custom_target(mlibc ALL meson install "--destdir" "${SYSROOT_DIR}" "--only-changed"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mlibc/build
                    COMMENT "Building and installing mlibc..."
                    VERBATIM)
