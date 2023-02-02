execute_process(COMMAND meson setup "-Dprefix=/usr" "--cross-file" "../${ARCH}-mieros.txt" "-Ddefault_library=static" build WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mlibc)

add_custom_target(mlibc ALL meson install "--destdir" "${SYSROOT_DIR}" "--only-changed"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mlibc/build
                    COMMENT "Building and installing mlibc..."
                    VERBATIM)

add_subdirectory(libopt)

