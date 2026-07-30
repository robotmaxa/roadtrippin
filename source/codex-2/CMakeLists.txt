cmake_minimum_required(VERSION 3.16)
project(RoadTripPlanner VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(road_trip_planner src/main.cpp)

if(MSVC)
  target_compile_options(road_trip_planner PRIVATE /W4)
else()
  target_compile_options(road_trip_planner PRIVATE -Wall -Wextra -Wpedantic)
endif()

