#include <print>
#include <string>
#include <vector>

#include "json_context/json_serializer.h"

struct test_variant {
    int n;
    int m;
};

struct test_struct {
    int foo;
    std::string hello;
    std::tuple<std::string, std::string, std::string, std::vector<std::string>> inner;
    std::variant<std::monostate, test_variant> variant;
};

namespace json_context {
    template<> struct serializer<int, int> {
        template<writers::writer W>
        void operator()(W &writer, int value, int ctx) {
            writer.write_value(value + ctx);
        }
    };
}

int main() {
    std::println("{}", json_context::to_string_json<test_struct, {
        .indent = 2,
        .colon_space = 1,
        .comma_space = 1
    }>(test_struct {
        .foo{99},
        .hello {"World"},
        .inner {
            "Foo", "Bar", "Baz", { "Lorem", "Ipsum" }
        },
        .variant {test_variant{ 42, 69 }}
    }));
}