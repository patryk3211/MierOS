set(SOURCES main.cpp scan.cpp)

add_executable(mdev ${SOURCES})

install(TARGETS mdev DESTINATION ${SYSROOT_DIR}/sbin)

