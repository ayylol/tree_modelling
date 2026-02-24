cmake_minimum_required(VERSION 3.22.2)
project(tree_strands)

# Export compile commands
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use modern C++
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

include_directories("${PROJECT_BINARY_DIR}" libs src)

# Libraries
set(DEFINITIONS _USE_MATH_DEFINES=1 GLM_FORCE_CXX14=1
    IMGUI_IMPL_OPENGL_LOADER_CUSTOM="glad/glad.h")

find_package(OpenGL REQUIRED)
set(LIBRARIES ${LIBRARIES} ${OPENGL_gl_LIBRARY})

# openmp install 
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(LIBRARIES ${LIBRARIES} OpenMP::OpenMP_CXX)
endif()

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)
set(LIBRARIES ${LIBRARIES} glfw)

# Copy all the resources and tell the build system to re-run CMAKE if one of them changes
file(GLOB_RECURSE resources RELATIVE ${CMAKE_SOURCE_DIR} resources/*)
foreach(file ${resources})
    configure_file(${file} ${file} COPYONLY)
endforeach(file)

# Making executable
# Original sources glob 
# file(GLOB sources src/*.cpp src/*.h libs/*.h libs/*.hpp libs/*.cpp libs/*.c libs/imgui/*.h libs/imgui/*.cpp)
# TODO: Make cleaner more inclusive regex
file(GLOB_RECURSE source
    ${CMAKE_CURRENT_LIST_DIR}/src/*\.[ch]
    ${CMAKE_CURRENT_LIST_DIR}/src/*\.[ch]pp
    )
file(GLOB libs libs/*.h libs/*.hpp libs/*.cpp libs/*.c libs/imgui/*.h libs/imgui/*.cpp)
file(GLOB sources ${source} ${libs})

add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${DEFINITIONS})
# FOR DEBUGGING
#target_compile_options(${PROJECT_NAME} PRIVATE -g -Wall -Wextra -pedantic) 
# FOR PERFORMANCE
target_compile_options(${PROJECT_NAME} PRIVATE -O3) 

