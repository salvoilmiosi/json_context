#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include "writer.h"

namespace json_context {

    template<typename T, typename Context> struct serializer;

    template<typename T, typename Context>
    concept serializable = is_complete<serializer<T, Context>>;

    template<typename T>
    concept writable_as_value = std::integral<T> || std::floating_point<T> || std::convertible_to<T, std::string_view>;

    template<typename T, writers::writer W, typename Context = no_context> requires serializable<T, Context>
    void serialize(W &writer, const T &value, const Context &context = {}) {
        serializer<T, Context> obj{};
        if constexpr (requires { obj(writer, value, context); }) {
            obj(writer, value, context);
        } else {
            obj(writer, value);
        }
    }

    template<writable_as_value T, typename Context>
    struct serializer<T, Context> {
        template<writers::writer W>
        void operator()(W &writer, const T &value) const {
            writer.write_value(value);
        }
    };

    template<std::ranges::range Range, typename Context>
    requires (
        serializable<std::ranges::range_value_t<Range>, Context>
        && !std::is_convertible_v<Range, std::string_view>
    )
    struct serializer<Range, Context> {
        template<writers::writer W>
        void operator()(W &writer, const Range &range, const Context &ctx) const {
            auto array = writer.begin_write_array();

            using value_type = std::ranges::range_value_t<Range>;
            for (auto &&value : range) {
                serialize(array, std::forward<decltype(value)>(value), ctx);
            }

            array.end();
        }
    };
    
    template<typename First, typename Second, typename Context>
    requires (serializable<First, Context> && serializable<Second, Context>)
    struct serializer<std::pair<First, Second>, Context> {
        template<writers::writer W>
        void operator()(W &writer, const std::pair<First, Second> &pair, const Context &ctx) const {
            auto array = writer.begin_write_array();

            serialize(array, pair.first, ctx);
            serialize(array, pair.second, ctx);

            array.end();
        }
    };

    template<typename Context, typename ... Ts>
    requires (serializable<Ts, Context> && ...)
    struct serializer<std::tuple<Ts ...>, Context> {
        template<writers::writer W>
        void operator()(W &writer, const std::tuple<Ts ...> &tuple, const Context &ctx) const {
            auto array = writer.begin_write_array();

            [&]<size_t ... Is>(std::index_sequence<Is ...>) {
                (serialize(array, std::get<Is>(tuple), ctx), ...);
            }(std::index_sequence_for<Ts ...>());

            array.end();
        }
    };

    template<aggregate T, typename Context>
    struct serializer<T, Context> {
        template<writers::writer W>
        void operator()(W &writer, const T &value, const Context &ctx) const {
            auto object = writer.begin_write_object();

            reflect::for_each<T>([&](auto I) {
                using member_type = reflect::member_type<I, T>;
                constexpr auto key = reflect::member_name<I, T>();
                object.write_key(key);
                serialize(object, reflect::get<I>(value), ctx);
            });

            object.end();
        }
    };

    template<typename Context, typename ... Ts> requires (serializable<Ts, Context> && ...)
    struct serializer<std::variant<Ts ...>, Context> {
        using variant_type = std::variant<Ts ...>;

        template<writers::writer W>
        void operator()(W &writer, const variant_type &value, const Context &ctx) const {
            auto object = writer.begin_write_object();

            std::visit([&](const auto &inner_value) {
                using member_type = std::remove_cvref_t<decltype(inner_value)>;
                constexpr auto key = reflect::type_name<member_type>();
                object.write_key(key);
                serialize(object, inner_value, ctx);
            }, value);

            object.end();
        }
    };

}

#endif