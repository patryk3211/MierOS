set(SOURCES src/main.cpp
            src/parser/parser_base.cpp)

add_executable(init ${SOURCES})
target_include_directories(init PRIVATE include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

install(TARGETS init DESTINATION ${SYSROOT_DIR}/sbin)
