set(CMAKE_CXX_COMPILER "${CROSS_DIR}/host/bin/${ARCH}-mieros-g++")
set(CMAKE_C_COMPILER "${CROSS_DIR}/host/bin/${ARCH}-mieros-gcc")

add_subdirectory(libs)
add_subdirectory(programs)
