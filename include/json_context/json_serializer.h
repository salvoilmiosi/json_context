#ifndef __JSON_SERIALIZER_H__
#define __JSON_SERIALIZER_H__

#include "json_writer.h"
#include "serializer.h"

namespace json_context {

    template<typename T, writers::json_writer_options Options = writers::json_writer_options{}, typename Context = no_context>
    requires serializable<T, Context>
    auto to_string_json(T &&value, const Context &ctx = {}) {
        std::string buf;
        writers::json_writer<std::string, Options> writer{buf};
        serializer<std::remove_cvref_t<T>, Context>{}(writer, std::forward<T>(value), ctx);
        return buf;
    }
}

#endif