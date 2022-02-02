cmake_minimum_required(VERSION 3.10)

# set the project name
project(WordleSolver)

add_compile_options(-Wno-c++11-extensions -g -O0)

file( GLOB RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.txt)
file(COPY ${RESOURCES}
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

# add the executable
add_executable(WorldleSolver Main.cpp)
