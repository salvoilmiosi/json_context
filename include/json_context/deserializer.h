#ifndef __DESERIALIZER_H__
#define __DESERIALIZER_H__

#include "reader.h"

#include "static_map.h"

namespace json_context {

    template<typename T, typename Context> struct deserializer;

    template<typename T, typename Context>
    concept deserializable = is_complete<deserializer<T, Context>>;

    template<typename T, readers::reader R, typename Context = no_context> requires deserializable<T, Context>
    T deserialize(R &reader, const Context &context = {}) {
        deserializer<T, Context> obj{};
        if constexpr (requires { obj(reader, context); }) {
            return obj(reader, context);
        } else {
            return obj(reader);
        }
    }

    template<std::integral T, typename Context  >
    struct deserializer<T, Context> {
        template<readers::reader R>
        T operator()(R &reader) const {
            if (auto value = reader.read_int()) {
                return reader.get_parser().parse_int(*value);
            }
            throw deserialize_error{"Expected integer"};
        }
    };

    template<std::floating_point T, typename Context  >
    struct deserializer<T, Context> {
        template<readers::reader R>
        T operator()(R &reader) const {
            if (auto value = reader.read_float()) {
                return reader.get_parser().parse_float(*value);
            }
            throw deserialize_error{"Expected float"};
        }
    };

    template<typename Context  >
    struct deserializer<std::string, Context> {
        template<readers::reader R>
        std::string operator()(R &reader) const {
            if (auto value = reader.read_string()) {
                return reader.get_parser().parse_string(*value);
            }
            throw deserialize_error{"Expected string"};
        }
    };

    template<typename T, typename Context>
    requires deserializable<T, Context>
    struct deserializer<std::vector<T>, Context> {
        template<readers::reader R>
        std::vector<T> operator()(R &reader, const Context &ctx) const {
            std::vector<T> result;

            auto opt_array = reader.begin_read_array();
            if (!opt_array) throw deserialize_error{"Expected array"};
            auto &array = *opt_array;

            while (!array.read_end()) {
                result.push_back(deserialize<T>(array, ctx));
            }

            return result;
        }
    };
    
    template<typename First, typename Second, typename Context>
    requires (deserializable<First, Context> && deserializable<Second, Context>)
    struct deserializer<std::pair<First, Second>, Context> {
        template<readers::reader R>
        std::pair<First, Second> operator()(R &reader, const Context &ctx) const {
            auto array = reader.begin_read_array();
            if (!array) throw deserialize_error{"Expected array"};

            auto first = deserialize<First>(*array, ctx);
            auto second = deserialize<Second>(*array, ctx);

            if (!array->read_end()) throw deserialize_error{"Expected array end"};

            return { std::move(first), std::move(second) };
        }
    };

    template<typename Context, typename ... Ts>
    requires (deserializable<Ts, Context> && ...)
    struct deserializer<std::tuple<Ts ...>, Context> {
        using tuple_type = std::tuple<Ts ...>;

        template<readers::reader R>
        tuple_type operator()(R &reader, const Context &ctx) const {
            auto array = reader.begin_read_array();
            if (!array) throw deserialize_error{"Expected array"};

            auto result = [&]<size_t ... Is>(std::index_sequence<Is ...>) {
                std::tuple<std::optional<Ts> ...> opts{};
                ((std::get<Is>(opts) = deserialize<Ts>(*array, ctx)), ...);

                return tuple_type{ std::move(std::get<Is>(opts)) ... };
            }(std::index_sequence_for<Ts ...>());

            if (!array->read_end()) throw deserialize_error{"Expected array end"};
            return result;
        }
    };

    template<aggregate T, typename Context>
    struct deserializer<T, Context> {
        template<readers::reader R, size_t ... Is>
        T deserialize_helper(std::index_sequence<Is ...>, R &reader, const Context &ctx) const {
            auto object = reader.begin_read_object();
            if (!object) throw deserialize_error{"Expected object"};
            
            using result_tuple_type = std::tuple<std::optional<reflect::member_type<Is, T>> ...>;
            result_tuple_type result{};

            static constexpr auto names_map = utils::make_static_map<std::string_view, size_t>({
                { reflect::member_name<Is, T>(), Is } ...
            });

            using object_reader = decltype(*object);
            using vtable_fun = void (*)(std::string_view key, object_reader &reader, const Context &ctx, result_tuple_type &result);
            static constexpr auto vtable = std::array<vtable_fun, sizeof...(Ts)> {
                [](std::string_view key, object_reader &reader, const Context &ctx, result_tuple_type &result) {
                    auto &member = std::get<Is>(result);
                    if (member.has_value()) throw deserialize_error{std::format("Duplicate field: {}", key)};
                    member = deserialize<reader::member_type<Is, T>>(reader, ctx);
                } ...
            };

            size_t count = 0;

            while (!object->read_end()) {
                auto key = object->read_key();
                if (!key) throw deserialize_error{"Expected key"};

                auto key_str = object->get_parser().parse_string(*key);

                auto key_it = names_map.find(key_str);
                if (key_it == names_map.end()) throw deserialize_error{std::format("Cannot find key {}", key_str)};

                vtable[key_it->second](key_str, *object, ctx, result);

                ++count;
            }

            if (count != sizeof...(Ts)) throw deserialize_error{"Field missing"};
            
            return T{ std::move(*(std::get<Is>(result))) ... };
        }

        template<readers::reader R>
        T operator()(R &reader, const Context &ctx) const {
            return deserialize_helper(std::make_index_sequence<reflect::size<T>()>(), reader, ctx);
        }
    };

    template<typename Context, typename ... Ts> requires (deserializable<Ts, Context> && ...)
    struct deserializer<std::variant<Ts ...>, Context> {
        using variant_type = std::variant<Ts ...>;

        template<readers::reader R>
        variant_type operator()(R &reader, const Context &ctx) const {
            auto object = reader.begin_read_object();
            if (!object) throw deserialize_error{"Expected object"};

            static constexpr auto names_map = []<size_t ... Is>(std::index_sequence<Is ...>) {
                return utils::make_static_map<std::string_view, size_t>({
                    { reflect::type_name<Ts>(), Is } ...
                });
            }(std::index_sequence_for<Ts ...>());

            auto key = object->read_key();
            if (!key) throw deserialize_error{"Expected key"};

            auto key_str = reader.get_parser().parse_string(*key);

            auto key_it = names_map.find(key_str);
            if (key_it == names_map.end()) throw deserialize_error{std::format("Cannot find key {}", key_str)};

            using object_reader = decltype(*object);
            using vtable_fun = variant_type (*)(object_reader &reader, const Context &ctx);
            static constexpr auto vtable = std::array<vtable_fun, sizeof...(Ts)> {
                [](object_reader &reader, const Context &ctx) -> variant_type {
                    return deserialize<Ts>(reader, ctx);
                } ...
            };

            auto result = vtable[key_it->second](*object, ctx);

            if (!object->read_end()) throw deserialize_error{"Expected object end"};

            return result;
        }
    };

}

#endif