project(rlfic)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

SET(CMAKE_CXX_FLAGS "-O3")
SET(CMAKE_C_FLAGS "-O3")
add_executable(${PROJECT_NAME} ${SRC_LIST})

