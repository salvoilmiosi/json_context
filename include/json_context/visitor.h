#ifndef __VISITOR_H__
#define __VISITOR_H__

#include <string>

namespace json_context::visitors {

    template<typename T>
    concept visitor_base = requires (T &v, std::string_view str, int i, float f) {
        v.write_value(str);
        v.write_value(nullptr);
        v.write_value(i);
        v.write_value(f);
    };

    template<typename T>
    concept inner_visitor = visitor_base<T> && requires (T &v) {
        v.end();
    };

    template<typename T>
    concept object_visitor = inner_visitor<T> && requires (T &v, std::string_view str) {
        v.write_key(str);
    };

    template<typename T>
    concept visitor = visitor_base<T> && requires (T &v) {
        { v.begin_write_array() } -> inner_visitor;
        { v.begin_write_object() } -> object_visitor;
    };
}

#endif