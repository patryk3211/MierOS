add_executable(test.mod main.cpp)

install(TARGETS test.mod DESTINATION ${SYSROOT_DIR}/lib/modules)
