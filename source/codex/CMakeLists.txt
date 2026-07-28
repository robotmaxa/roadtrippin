cmake_minimum_required(VERSION 3.16)

project(roadtrip_planner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(roadtrip_planner
    src/main.cpp
    src/planner.cpp
)
target_include_directories(roadtrip_planner PRIVATE include)

add_executable(planner_tests
    tests/test_planner.cpp
    src/planner.cpp
)
target_include_directories(planner_tests PRIVATE include)

file(COPY data DESTINATION ${CMAKE_BINARY_DIR})

enable_testing()
add_test(NAME planner_tests COMMAND planner_tests)
