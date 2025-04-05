#include <iostream>
#include <cassert>
#include <vector>
#include <map>

#include "json_context/json_serializer.h"

void test_json_to_string1() {
    struct test_variant {
        float n;
        int m;
    };
    
    struct test_struct {
        int foo;
        std::string hello;
        std::tuple<std::string, std::vector<std::string>> inner;
        std::variant<std::monostate, test_variant> variant;
    };

    test_struct sample_data {
        .foo {99},
        .hello {"World"},
        .inner {
            "Foo", { "Bar", "Baz" }
        },
        .variant {test_variant{ std::numeric_limits<float>::max(), 42 }}
    };

    std::string expected_json = R"({
  "foo": 99,
  "hello": "World",
  "inner": [
    "Foo",
    [
      "Bar",
      "Baz"
    ]
  ],
  "variant": {
    "test_variant": {
      "n": 3.4028235e+38,
      "m": 42
    }
  }
})";

    std::string result = json_context::to_string_json<test_struct, {
        .indent = 2,
        .colon_space = 1
    }>(sample_data);

    std::cout << result << '\n';

    assert(result == expected_json);
}

void test_json_to_string2() {
    struct test_struct {
        std::map<int, int> value;
    };

    test_struct sample_data{
        .value {
            { 1, 2 }, { 3, 4 }, { 5, 6}
        }
    };

    std::cout << json_context::to_string_json(sample_data) << '\n';
}

int main() {
    test_json_to_string1();
    test_json_to_string2();
}