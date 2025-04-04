#ifndef __JSON_VISITOR_H__
#define __JSON_VISITOR_H__

#include <format>

#include "writer.h"

namespace json_context::visitors {

    template<writers::writer W>
    class json_visitor {
    private:
        W &writer;

    public:
        explicit json_visitor(W &writer): writer{writer} {}

        void write_direct(std::string_view value) {
            writer.write(value);
        }

        void write_value(std::nullptr_t) {
            write_direct("null");
        }

        template<std::integral T>
        void write_value(T value) {
            write_direct(std::format("{}", value));
        };

        template<std::floating_point T>
        void write_value(T value) {
            write_direct(std::format("{}", value));
        };

        void write_value(bool value) {
            if (value) {
                write_direct("true");
            } else {
                write_direct("false");
            }
        }

        template<std::convertible_to<std::string_view> T>
        void write_value(T &&value) {
            // TODO fix character escapes
            write_direct(std::format("\"{}\"", std::string_view{std::forward<T>(value)}));
        }

    private:
        
        class array_visitor {
        private:
            json_visitor &instance;
            bool first = true;

            void write_comma() {
                if (first) {
                    first = false;
                } else {
                    instance.write_direct(",");
                }
            }

        public:
            explicit array_visitor(json_visitor &instance)
                : instance{instance}
            {
                instance.write_direct("[");
            }
        
            void write_value(auto &&value) {
                write_comma();
                instance.write_value(std::forward<decltype(value)>(value));
            }

            auto begin_write_array() {
                write_comma();
                return instance.begin_write_array();
            }

            auto begin_write_object() {
                write_comma();
                return instance.begin_write_object();
            }

            void end() {
                instance.write_direct("]");
            }
        };
        
        class object_visitor {
        private:
            json_visitor &instance;
            bool first = true;

        public:
            explicit object_visitor(json_visitor &instance)
                : instance{instance}
            {
                instance.write_direct("{");
            }

            void write_key(std::string_view key) {
                if (first) {
                    first = false;
                } else {
                    instance.write_direct(",");
                }

                instance.write_value(key);
                instance.write_direct(":");
            }
        
            void write_value(auto &&value) {
                instance.write_value(std::forward<decltype(value)>(value));
            }

            auto begin_write_array() {
                return instance.begin_write_array();
            }

            auto begin_write_object() {
                return instance.begin_write_object();
            }

            void end() {
                instance.write_direct("}");
            }
        };

    public:

        auto begin_write_array() {
            return array_visitor{*this};
        }

        auto begin_write_object() {
            return object_visitor{*this};
        }
    };
    
}

#endif