cmake_minimum_required(VERSION 3.22.2)

project(tree_strands)

file(GLOB sources src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${sources})

