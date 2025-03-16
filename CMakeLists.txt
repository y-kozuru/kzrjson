cmake_minimum_required(VERSION 3.28)
project(kzrjson C)

file(GLOB SRCS *.c)
add_executable(${PROJECT_NAME} ${SRCS})

# Set C11
target_compile_features(${PROJECT_NAME} PUBLIC
	c_std_11
)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_compile_options(${PROJECT_NAME} PUBLIC
	-Wall
	-pedantic-errors
	-g3
)
