set(SOURCES src/parser/parser_base.cpp)

add_library(init-utils ${SOURCES})
target_include_directories(init-utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

