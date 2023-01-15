
add_executable(test_user test.c)
install(TARGETS test_user DESTINATION ${SYSROOT_DIR}/)
add_dependencies(test_user mlibc)
