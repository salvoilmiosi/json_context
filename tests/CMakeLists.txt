# Enable testing
enable_testing()

add_executable(test_to_string_json to_string_json.cpp)
target_link_libraries(test_to_string_json json_context)

# Add the test
add_test(NAME TestToStringJson COMMAND test_to_string_json)