#ifndef __DESERIALIZER_H__
#define __DESERIALIZER_H__

#include "reader.h"

#include "static_map.h"

namespace json_context {

    template<typename T, typename Context> struct deserializer;

    template<typename T, typename Context>
    concept deserializable = is_complete<deserializer<T, Context>>;

    template<typename T, readers::reader R, typename Context = no_context> requires deserializable<T, Context>
    std::optional<T> deserialize(R &reader, const Context &context = {}) {
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
        std::optional<T> operator()(R &reader) const {
            if (auto value = reader.read_int()) {
                return reader.get_parser().parse_int(*value);
            }
            return std::nullopt;
        }
    };

    template<std::floating_point T, typename Context  >
    struct deserializer<T, Context> {
        template<readers::reader R>
        std::optional<T> operator()(R &reader) const {
            if (auto value = reader.read_float()) {
                return reader.get_parser().parse_float(*value);
            }
            return std::nullopt;
        }
    };

    template<typename Context  >
    struct deserializer<std::string, Context> {
        template<readers::reader R>
        std::optional<std::string> operator()(R &reader) const {
            if (auto value = reader.read_string()) {
                return reader.get_parser().parse_string(*value);
            }
            return std::nullopt;
        }
    };

    template<typename T, typename Context>
    requires deserializable<T, Context>
    struct deserializer<std::vector<T>, Context> {
        template<readers::reader R>
        std::optional<std::vector<T>> operator()(R &reader, const Context &ctx) const {
            std::vector<T> result;

            auto array = reader.begin_read_array();
            if (!array) return std::nullopt;

            while (!array->read_end()) {
                auto value = deserialize<T>(*array, ctx);
                if (!value) return std::nullopt;

                result.push_back(std::move(*value));
            }

            return std::move(result);
        }
    };
    
    template<typename First, typename Second, typename Context>
    requires (deserializable<First, Context> && deserializable<Second, Context>)
    struct deserializer<std::pair<First, Second>, Context> {
        template<readers::reader R>
        std::optional<std::pair<First, Second>> operator()(R &reader, const Context &ctx) const {
            auto array = reader.begin_read_array();
            if (!array) return std::nullopt;

            auto first = deserialize<First>(*array, ctx);
            if (!first) return std::nullopt;

            auto second = deserialize<Second>(*array, ctx);
            if (!second) return std::nullopt;

            if (!array->read_end()) return std::nullopt;

            return { std::in_place, std::move(first), std::move(second) };
        }
    };

    template<typename Context, typename ... Ts>
    requires (deserializable<Ts, Context> && ...)
    struct deserializer<std::tuple<Ts ...>, Context> {
        using tuple_type = std::tuple<Ts ...>;

        template<readers::reader R>
        std::optional<tuple_type> operator()(R &reader, const Context &ctx) const {
            auto array = reader.begin_read_array();
            if (!array) return std::nullopt;

            std::tuple<std::optional<Ts> ...> result{};

            return [&]<size_t ... Is>(std::index_sequence<I ...>) -> std::optional<tuple_type> {
                if (((std::get<Is>(result) = deserialize<std::tuple_element_t<Is, tuple_type>>(*array, ctx)).has_value() && ...) && array->read_end()) {
                    return { std::in_place, std::move(*(std::get<Is>(result))) ... };
                } else {
                    return std::nullopt;
                }
            }(std::index_sequence_for<Ts ...>());
        }
    };

    template<aggregate T, typename Context>
    struct deserializer<T, Context> {
        template<readers::reader R, size_t ... Is>
        std::optional<T> deserialize_helper(std::index_sequence<Is ...>, R &reader, const Context &ctx) const {
            auto object = reader.begin_read_object();
            if (!object) return std::nullopt;
            
            using result_tuple_type = std::tuple<std::optional<reflect::member_type<Is, T>> ...>;
            result_tuple_type result{};

            static constexpr auto names_map = utils::make_static_map<std::string_view, size_t>({
                { reflect::member_name<Is, T>(), Is } ...
            });

            using vtable_fun = bool (*)(R &reader, const Cotnext &ctx, result_tuple_type &result);
            static constexpr auto vtable = std::array<vtable_fun, sizeof...(Ts)> {
                [](R &reader, const Context &ctx, result_tuple_type &result) -> bool {
                    auto &member = std::get<Is>(result);
                    if (member.has_value()) {
                        return false;
                    }
                    member = deserialize<reader::member_type<Is, T>>(reader, ctx);
                    return true;
                } ...
            };

            size_t count = 0;

            while (!object->read_end()) {
                auto key = object->read_key();
                if (!key) return std::nullopt;

                auto key_str = reader.get_parser().parse_string(*key);

                auto key_it = names_map.find(key_str);
                if (key_it == names_map.end()) return std::nullopt;

                if (!vtable[key_it->second](reader, ctx, result)) return std::nullopt;

                ++count;
            }

            if (count != sizeof...(Ts)) return std::nullopt;
            
            return { std::in_place, std::move(*(std::get<Is>(result))) ... };
        }

        template<readers::reader R>
        std::optional<T> operator()(R &reader, const Context &ctx) const {
            return deserialize_helper(std::make_index_sequence<reflect::size<T>()>(), reader, ctx);
        }
    };

    template<typename Context, typename ... Ts> requires (deserializable<Ts, Context> && ...)
    struct deserializer<std::variant<Ts ...>, Context> {
        using variant_type = std::variant<Ts ...>;

        template<readers::reader R>
        std::optional<variant_type> operator()(R &reader, const Context &ctx) const {
            auto object = reader.begin_read_object();
            if (!object) return std::nullopt;

            static constexpr auto names_map = []<size_t ... Is>(std::index_sequence<Is ...>) {
                return utils::make_static_map<std::string_view, size_t>({
                    { reflect::type_name<Ts>(), Is } ...
                });
            }(std::index_sequence_for<Ts ...>());

            auto key = object->read_key();
            if (!key) return std::nullopt;

            auto key_str = reader.get_parser().parse_string(*key);

            auto key_it = names_map.find(key_str);
            if (key_it == names_map.end()) return std::nullopt;

            using object_reader = decltype(*object);
            using vtable_fun = std::optional<variant_type>(*)(object_reader &reader, const Context &ctx);
            static constexpr auto vtable = std::array<vtable_fun, sizeof...(Ts)> {
                [](object_reader &reader, const Context &ctx) -> std::optional<variant_type> {
                    auto value = deserialize<Ts>(reader, ctx);
                    if (value) return { std::move(*value) };
                    return std::nullopt;
                } ...
            };

            auto result = vtable[key_it->second](*object, ctx);
            if (!result) return std::nullopt;

            if (!object->read_end()) return std::nullopt;

            return result;
        }
    };

}

#endif