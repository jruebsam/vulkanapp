cmake_minimum_required(VERSION 3.1)
project(VulkanDemo)

set(BIN_NAME "vkdemo")
set(NDEBUG)

set(shader_source "${CMAKE_SOURCE_DIR}/shaders")
set(shader_destination "${CMAKE_CURRENT_BINARY_DIR}/shaders")

set(texture_source "${CMAKE_SOURCE_DIR}/textures")
set(texture_destination "${CMAKE_CURRENT_BINARY_DIR}/textures")

set(models_source "${CMAKE_SOURCE_DIR}/models")
set(models_destination "${CMAKE_CURRENT_BINARY_DIR}/models")

file(GLOB SOURCES "src/*.cpp")
include_directories("include")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(assimp REQUIRED)


find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall")
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${BIN_NAME} ${SOURCES})

target_link_libraries(${BIN_NAME} glfw)
target_link_libraries(${BIN_NAME} vulkan)
target_link_libraries(${BIN_NAME} assimp)


# Compile shaders
add_custom_command(TARGET ${BIN_NAME} PRE_BUILD
COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/shaders/compile.sh)

add_custom_command(TARGET ${BIN_NAME}
COMMAND ${CMAKE_COMMAND} -E create_symlink ${shader_source} ${shader_destination}
DEPENDS ${shader_destination}
COMMENT "symbolic link resource folder from ${shader_source} => ${shader_destination}")

add_custom_command(TARGET ${BIN_NAME}
COMMAND ${CMAKE_COMMAND} -E create_symlink ${texture_source} ${texture_destination}
DEPENDS ${texture_destination}
COMMENT "symbolic link resource folder from ${texture_source} => ${texture_destination}")

add_custom_command(TARGET ${BIN_NAME}
COMMAND ${CMAKE_COMMAND} -E create_symlink ${models_source} ${models_destination}
DEPENDS ${models_destination}
COMMENT "symbolic link resource folder from ${models_source} => ${models_destination}")
