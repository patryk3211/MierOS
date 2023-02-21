set(SOURCES
    src/main.cpp
    src/uevent_handler.cpp
)

add_executable(init ${SOURCES})
target_include_directories(init PRIVATE include)
target_link_libraries(init init-utils)

install(TARGETS init DESTINATION ${SYSROOT_DIR}/sbin)

