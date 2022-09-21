execute_process(COMMAND meson build WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mlibc)

add_custom_target(mlibc ALL meson install "--destdir" "${SYSROOT_DIR}" "--only-changed"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mlibc/build
                    COMMENT "Building and installing mlibc..."
                    VERBATIM)
