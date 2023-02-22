add_custom_target(ports)

function(add_port NAME)
    add_custom_target(port_${NAME} env
                      "SYSROOT=${SYSROOT_DIR}"
                      "CC=${CMAKE_C_COMPILER}"
                      "CFLAGS=${CMAKE_C_FLAGS}"
                      "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}"
                      ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}/make # Execute the make file in port's directory
                      COMMENT "Making ${NAME} port"
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${NAME})
    add_dependencies(ports port_${NAME})
endfunction(add_port)

add_port(bash)
add_port(dash)

