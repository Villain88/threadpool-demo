cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")

project(signature)
set(SOURCES "signature.cpp"
            "blockdata.cpp"
            "md5.cpp"
            "main.cpp"
            "reader.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} -lpthread)
