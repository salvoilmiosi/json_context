cmake_minimum_required(VERSION 3.13.0)
project(json_context VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# bang server executable

add_executable(json_context "")

target_include_directories(json_context PRIVATE src)

add_subdirectory(src)

# external dependencies

# reflect

add_subdirectory(external/reflect)
target_link_libraries(json_context PRIVATE reflect)