#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <string>
#include <ranges>
#include <tuple>
#include <variant>
#include <reflect>

#include "writer.h"

namespace json_context {

    template<typename T>
    concept is_complete = requires(T self) { sizeof(self); };

    template<typename T, typename Context> struct serializer;

    template<typename T, typename Context>
    concept serializable = is_complete<serializer<T, Context>>;

    struct no_context {};

    template<std::integral T, typename Context>
    struct serializer<T, Context> {
        template<writers::writer W>
        void operator()(W &writer, T value, const Context &ctx) const {
            writer.write_value(value);
        }
    };

    template<std::floating_point T, typename Context>
    struct serializer<T, Context> {
        template<writers::writer W>
        void operator()(W &writer, T value, const Context &ctx) const {
            writer.write_value(value);
        }
    };

    template<typename Context>
    struct serializer<bool, Context> {
        template<writers::writer W>
        void operator()(W &writer, bool value, const Context &ctx) const {
            writer.write_value(value);
        }
    };

    template<std::convertible_to<std::string_view> T, typename Context>
    struct serializer<T, Context> {
        template<writers::writer W>
        void operator()(W &writer, const T &value, const Context &ctx) const {
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
                serializer<value_type, Context>{}(array, std::forward<decltype(value)>(value), ctx);
            }

            array.end();
        }
    };

    template<typename Context, typename ... Ts>
    requires (serializable<Ts, Context> && ...)
    struct serializer<std::tuple<Ts ...>, Context> {
        template<writers::writer W>
        void operator()(W &writer, const std::tuple<Ts ...> &tuple, const Context &ctx) const {
            auto array = writer.begin_write_array();

            auto serialize_inner = [&](auto &&value) {
                using value_type = std::remove_cvref_t<decltype(value)>;
                serializer<value_type, Context>{}(array, std::forward<decltype(value)>(value), ctx);
            };

            [&]<size_t ... Is>(std::index_sequence<Is ...>) {
                (serialize_inner(std::get<Is>(tuple)), ...);
            }(std::index_sequence_for<Ts ...>());

            array.end();
        }
    };

    template<typename T>
    concept aggregate = std::is_aggregate_v<T>;

    template<aggregate T, typename Context>
    struct serializer<T, Context> {
        template<writers::writer W>
        void operator()(W &writer, const T &value, const Context &ctx) const {
            auto object = writer.begin_write_object();

            reflect::for_each<T>([&](auto I) {
                using member_type = reflect::member_type<I, T>;
                constexpr auto key = reflect::member_name<I, T>();
                object.write_key(key);
                serializer<member_type, Context>{}(object, reflect::get<I>(value), ctx);
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
                serializer<member_type, Context>{}(object, inner_value, ctx);
            }, value);

            object.end();
        }
    };

}

#endif