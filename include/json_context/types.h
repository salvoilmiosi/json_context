#ifndef __TYPES_H__
#define __TYPES_H__

#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <variant>
#include <ranges>
#include <reflect>
#include <stdexcept>
#include <format>

namespace json_context {

    template<typename T>
    concept is_complete = requires(T self) { sizeof(self); };

    struct no_context {};

    template<typename T>
    concept aggregate = std::is_aggregate_v<T>;

    struct json_writer_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct deserialize_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}

#endif