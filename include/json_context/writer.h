#ifndef __WRITER_H__
#define __WRITER_H__

#include <string>

namespace json_context::writers {

    template<typename T>
    concept output_buffer = requires (T obj, std::string_view data) {
        obj.append(data);
    };

    template<typename T>
    concept writer_base = requires (T &v, std::string_view str, int i, float f) {
        v.write_value(str);
        v.write_value(nullptr);
        v.write_value(i);
        v.write_value(f);
    };

    template<typename T>
    concept inner_writer = writer_base<T> && requires (T &v) {
        v.end();
    };

    template<typename T>
    concept object_writer = inner_writer<T> && requires (T &v, std::string_view str) {
        v.write_key(str);
    };

    template<typename T>
    concept writer = writer_base<T> && requires (T &v) {
        { v.begin_write_array() } -> inner_writer;
        { v.begin_write_object() } -> object_writer;
    };
}

#endif