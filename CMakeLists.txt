cmake_minimum_required(VERSION 3.13.0)
project(json_context VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# bang server executable

add_library(json_context INTERFACE)

target_include_directories(json_context INTERFACE include)

# external dependencies

# reflect

add_subdirectory(external/reflect)
target_link_libraries(json_context INTERFACE reflect)

# tests

add_subdirectory(tests)