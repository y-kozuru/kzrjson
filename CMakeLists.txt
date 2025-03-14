cmake_minimum_required(VERSION 3.28)
project(kzrjson CXX)

file(GLOB SRCS *.cpp)
add_executable(${PROJECT_NAME} ${SRCS})

# Set C++20
target_compile_options(${PROJECT_NAME} PUBLIC
	-Wall
	-pedantic-errors
	-g3
)

target_compile_features(${PROJECT_NAME} PUBLIC
	cxx_std_20
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
