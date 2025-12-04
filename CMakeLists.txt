cmake_minimum_required(VERSION 4.0)
project(HuffmanEncrypt)

set(CMAKE_CXX_STANDARD 20)

add_executable(HuffmanEncrypt main.cpp)
find_package(OpenMP REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")