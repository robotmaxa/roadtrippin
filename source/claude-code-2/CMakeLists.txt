
cmake_minimum_required(VERSION 3.10)
project(road_trip_optimizer CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
 
add_executable(road_trip src/main.cpp)
target_include_directories(road_trip PRIVATE include)

# Copy the location database next to the binary for convenience.
add_custom_command(TARGET road_trip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:road_trip>/data)