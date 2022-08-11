cmake_minimum_required(VERSION 3.23)

project(Cheese)

find_package(raylib REQUIRED)

file(GLOB SRCS src/*)
add_executable(Cheese ${SRCS})

target_link_libraries(Cheese raylib)
target_include_directories(Cheese PRIVATE ${RAYLIB_INCLUDE_DIRS})
