cmake_minimum_required(VERSION 3.1)
project(VulkanDemo)

set(BIN_NAME "vkdemo")
set(NDEBUG)

set(source "${CMAKE_SOURCE_DIR}/shaders")
set(destination "${CMAKE_CURRENT_BINARY_DIR}/shaders")

file(GLOB SOURCES "src/*.cpp")
include_directories("include")


find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall")
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${BIN_NAME} ${SOURCES})

target_link_libraries(${BIN_NAME} glfw)
target_link_libraries(${BIN_NAME} vulkan)


# Compile shaders
add_custom_command(TARGET ${BIN_NAME} PRE_BUILD
COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/shaders/compile.sh)

add_custom_command(TARGET ${BIN_NAME}
COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
DEPENDS ${destination}
COMMENT "symbolic link resource folder from ${source} => ${destination}")
