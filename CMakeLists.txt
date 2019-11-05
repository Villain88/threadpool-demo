cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)

project(signature)
set(SOURCES "signature.cpp"
            "blockdata.cpp"
            "md5.cpp"
            "main.cpp"
            "reader.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} -lboost_system -lboost_thread -lpthread)
