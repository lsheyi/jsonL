cmake_minimum_required(VERSION 3.15)
project(benchmark)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE "Debug")
file(GLOB SRC ${PROJECT_SOURCE_DIR}/*.cpp)

add_executable(result ${SRC})
target_link_libraries(result benchmark pthread)