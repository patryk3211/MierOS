set(CMAKE_C_COMPILER "${CROSS_DIR}/host-llvm/bin/clang")
set(CMAKE_CXX_COMPILER "${CROSS_DIR}/host-llvm/bin/clang++")

set(CLANG_FLAGS "-g -O1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_FLAGS}")

# Force libc ctors on the beginning of the .init_array sections
# This is needed because otherwise libc++ will be using stdio file descriptors
# AFTER they have been freed by libc destructors, however I believe that this
# is only required when static linking the c library. When we are dynamically
# linking, the order of initalization should be correct as long as libc is
# the first dependency in DT_NEEDED entries.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Tstatic-libc-ctor.ld")

add_subdirectory(libs)
add_subdirectory(programs)
