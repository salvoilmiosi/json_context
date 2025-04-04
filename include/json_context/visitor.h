#ifndef __VISITOR_H__
#define __VISITOR_H__

#include <string>

namespace json_context::visitors {

    template<typename T>
    concept visitor_base = requires (T &v) {
        v.write_value(std::declval<std::string_view>());
        v.write_value(nullptr);
        v.write_value(std::declval<int>());
        v.write_value(std::declval<float>());
    };

    template<typename T>
    concept inner_visitor = visitor_base<T> && requires (T &v) {
        v.end();
    };

    template<typename T>
    concept visitor = visitor_base<T> && requires (T &v) {
        { v.begin_write_array() } -> inner_visitor;
        { v.begin_write_object() } -> inner_visitor;
    };
}

#endif