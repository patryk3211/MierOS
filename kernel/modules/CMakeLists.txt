set(CMAKE_C_FLAGS "${KERNEL_C_FLAGS} -fno-plt")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${KERNEL_CXX_FLAGS}")

set(LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
set(EXIT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/module.cpp)

function(add_kernel_module MOD_NAME MOD_SOURCES)
    set(_MOD_SOURCES ${MOD_SOURCES} ${ARGN})
    add_executable(${MOD_NAME}.mod ${_MOD_SOURCES} ${EXIT_FILE})

    set_target_properties(${MOD_NAME}.mod PROPERTIES
                      PREFIX ""
                      SUFFIX "")

    target_link_libraries(${MOD_NAME}.mod $<TARGET_FILE:kernel.bin>)
    add_dependencies(${MOD_NAME}.mod kernel.bin)

    target_link_options(${MOD_NAME}.mod PRIVATE -T${LD_SCRIPT} -z max-page-size=0x1000 -pie -Wl,-export-dynamic -Wl,--no-dynamic-linker)

    install(TARGETS ${MOD_NAME}.mod DESTINATION ${SYSROOT_DIR}/lib/modules)
endfunction(add_kernel_module)

add_subdirectory(test)
add_subdirectory(pci)
add_subdirectory(ahci)
add_subdirectory(ext2)
