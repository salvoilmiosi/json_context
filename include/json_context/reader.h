#ifndef __WRITER_H__
#define __WRITER_H__

#include "types.h"

namespace json_context::readers {

    template<typename T>
    concept parser = requires (T &v, std::string_view str) {
        { v.parse_string(str) } -> std::convertible_to<std::string>;
        { v.parse_int(str) } -> std::integral;
        { v.parse_float(str) } -> std::floating_point;
    };

    template<typename T>
    concept optional_string_view = std::convertible_to<T, bool> && requires (const T &v) {
        { *v } -> std::convertible_to<std::string_view>;
    };

    template<typename T>
    concept reader_base = requires (T &v) {
        { v.read_string() } -> optional_string_view;
        { v.read_null() } -> optional_string_view;
        { v.read_int() } -> optional_string_view;
        { v.read_float() } -> optional_string_view;
        { v.get_parser() } -> parser;
    };

    template<typename T>
    concept inner_reader = reader_base<T> && requires (T &v) {
        { v.read_end() } -> std::convertible_to<bool>;
    };

    template<typename T>
    concept object_reader = inner_reader<T> && requires (T &v) {
        { v.read_key() } -> optional_string_view;
    };

    template<typename T>
    concept optional_inner_reader = std::convertible_to<T, bool> && requires (const T &v) {
        { *v } -> inner_reader;
    };

    template<typename T>
    concept optional_object_reader = std::convertible_to<T, bool> && requires (const T &v) {
        { *v } -> object_reader;
    };

    template<typename T>
    concept reader = reader_base<T> && requires (T &v) {
        { v.begin_read_array() } -> optional_inner_reader;
        { v.begin_read_object() } -> optional_object_reader;
    };
}

#endif