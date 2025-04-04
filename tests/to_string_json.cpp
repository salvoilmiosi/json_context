#include <iostream>
#include <cassert>
#include <vector>

#include "json_context/json_serializer.h"

int main() {
    struct test_variant {
        int n;
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
        .variant {test_variant{ 42, 69 }}
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
      "n": 42,
      "m": 69
    }
  }
})";

    std::string result = json_context::to_string_json<test_struct, {
        .indent = 2,
        .colon_space = 1
    }>(sample_data);

    assert(result == expected_json);
}
