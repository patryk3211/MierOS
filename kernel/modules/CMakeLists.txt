link_libraries($<TARGET_FILE:kernel.bin>)

set(CMAKE_C_FLAGS "-g -fvisibility=default -nostdlib -ffreestanding -Wall -Wextra -mcmodel=large -fPIC -Wl,-export-dynamic -fno-plt -mno-red-zone")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti -fno-use-cxa-atexit")

add_link_options(-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -z max-page-size=0x1000 -pie -Wl,--no-dynamic-linker)

add_subdirectory(test)
add_subdirectory(pci)
add_subdirectory(ahci)
