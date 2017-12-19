cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0048 NEW)

project(aioccc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(cart_bin cart.cpp)
set_target_properties(cart_bin PROPERTIES OUTPUT_NAME cart)
target_link_libraries(cart_bin ${OpenCV_LIBS} clang)

install(TARGETS cart_bin RUNTIME DESTINATION bin)
