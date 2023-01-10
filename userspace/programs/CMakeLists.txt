
add_executable(test_user test.c)
install(TARGETS test_user DESTINATION ${SYSROOT_DIR}/)
