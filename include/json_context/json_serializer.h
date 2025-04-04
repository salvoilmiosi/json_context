#ifndef __JSON_SERIALIZER_H__
#define __JSON_SERIALIZER_H__

#include "json_visitor.h"
#include "serializer.h"

namespace json_context {

    template<typename T, typename Context = no_context>
    requires serializable<T, Context>
    auto to_string_json(T &&value, const Context &ctx = {}) {
        std::string buf;
        writers::string_writer writer{buf};
        visitors::json_visitor visitor{writer};
        serializer<std::remove_cvref_t<T>, Context>{}(visitor, std::forward<T>(value), ctx);
        return buf;
    }
}

#endif