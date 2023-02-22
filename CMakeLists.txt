cmake_minimum_required(VERSION 3.10)

project(MierOS VERSION 0.0.1)

enable_language(C)
enable_language(CXX)
enable_language(ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Always make sure to export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ARCH "x86_64" CACHE STRING "Set the kernel's target architecture")
set(DEBUG OFF CACHE BOOL "Set if build should include some debugging features")
option(TESTING "Enable kernel and module testing" ON)

set(CROSS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cross")
set(SYSROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sysroot")

add_subdirectory(kernel)
add_subdirectory(userspace)

file(COPY ${CMAKE_SOURCE_DIR}/scripts/make_initrd.sh DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/scripts/copy_sysroot.sh DESTINATION ${CMAKE_BINARY_DIR})

add_custom_target(copy_sysroot "sh" "copy_sysroot.sh" "${SYSROOT_DIR}" "${CMAKE_SOURCE_DIR}/pass.txt"
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

set(INITRD_MODULES kernel/modules/pci/pci.mod
                   kernel/modules/test/test.mod
                   kernel/modules/ahci/ahci.mod
                   kernel/modules/ext2/ext2.mod
                   kernel/modules/pc_serial/pc_serial.mod)

set(INITRD_BINARIES userspace/programs/init/init
                    userspace/programs/system/modprobe/modprobe
                    userspace/programs/system/mdev/mdev)

set(INITRD_MAPPINGS
# Misc files
    ${CMAKE_SOURCE_DIR}/kernel/modules/modules.init=lib/modules/modules.init
    ${CMAKE_SOURCE_DIR}/kernel/modules/modules.alias=lib/modules/modules.alias
    ${CMAKE_SOURCE_DIR}/kernel/modules/modules.dep=lib/modules/modules.dep
    ${SYSROOT_DIR}/etc/fstab=etc/fstab
# Modules
    kernel/modules/pci/pci.mod=lib/modules/bus/pci.mod
    kernel/modules/ahci/ahci.mod=lib/modules/block/ahci.mod
    kernel/modules/ext2/ext2.mod=lib/modules/fs/ext2.mod
    kernel/modules/pc_serial/pc_serial.mod=lib/modules/tty/pc_serial.mod
# Executables
    userspace/programs/init/main/init=sbin/init
    userspace/programs/system/modprobe/modprobe=sbin/modprobe
    userspace/programs/system/mdev/mdev=sbin/mdev

    ${SYSROOT_DIR}/usr/bin/dash=bin/dash
)

add_custom_target(initrd.img ALL "sh" "${CMAKE_SOURCE_DIR}/scripts/make_initrd.sh"
        "${INITRD_MAPPINGS}"
        "${SYSROOT_DIR}/initrd.img"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} VERBATIM DEPENDS ${INITRD_MODULES} ${INITRD_BINARIES})
