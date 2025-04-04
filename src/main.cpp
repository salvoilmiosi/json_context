#include <print>
#include <string>
#include <vector>

#include "json_context/json_serializer.h"

struct test_struct {
    std::string hello;
    std::tuple<std::string, std::string, std::string, std::vector<std::string>> inner;
};

int main() {
    std::println("{}", json_context::to_string_json(test_struct {
        .hello {"World"},
        .inner {
            "Foo", "Bar", "Baz", { "Lorem", "Ipsum" }
        }
    }));
}