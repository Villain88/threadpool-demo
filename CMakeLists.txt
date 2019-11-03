cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)

project(signature)
add_executable(${PROJECT_NAME} "blockdata.hpp" "md5.cpp" "main.cpp")
target_link_libraries(${PROJECT_NAME} -lboost_system -lboost_thread -lpthread)
