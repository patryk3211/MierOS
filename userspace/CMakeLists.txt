#set(CMAKE_CXX_COMPILER "${CROSS_DIR}/host/bin/${ARCH}-mieros-g++")
#set(CMAKE_C_COMPILER "${CROSS_DIR}/host/bin/${ARCH}-mieros-gcc")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

#set(CLANG_FLAGS)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# Need that for system includes to be put into compile_commands and for
# clangd and nvim to work correctly
include_directories("${CROSS_DIR}/host/${ARCH}-mieros/include")

add_subdirectory(libs)
add_subdirectory(programs)
