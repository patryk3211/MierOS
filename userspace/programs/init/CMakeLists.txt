set(SOURCES src/main.cpp
            src/parser/parser_base.cpp)

add_executable(init ${SOURCES})
target_include_directories(init PRIVATE include)

install(TARGETS init DESTINATION ${SYSROOT_DIR}/sbin)
