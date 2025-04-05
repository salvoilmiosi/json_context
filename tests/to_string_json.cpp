#include <iostream>
#include <cassert>
#include <vector>

#include "json_context/json_serializer.h"

int main() {
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
