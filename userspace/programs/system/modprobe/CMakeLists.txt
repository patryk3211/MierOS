add_executable(modprobe main.cpp)
target_link_libraries(modprobe opt)

install(TARGETS modprobe DESTINATION ${SYSROOT_DIR}/sbin)

