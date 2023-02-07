set(CMAKE_C_COMPILER "${CROSS_DIR}/host-llvm/bin/clang")
set(CMAKE_CXX_COMPILER "${CROSS_DIR}/host-llvm/bin/clang++")

set(CLANG_FLAGS "-g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_FLAGS}")

add_subdirectory(libs)
add_subdirectory(programs)
