#ifndef __JSON_VISITOR_H__
#define __JSON_VISITOR_H__

#include <format>

#include "writer.h"

namespace json_context::visitors {
    
    struct json_visitor_options {
        int indent = 0;
        int colon_space = 0;
        int comma_space = 0;
    };

    template<writers::writer W>
    class json_visitor {
    private:
        W &writer;
        json_visitor_options options;

    public:
        explicit json_visitor(W &writer, json_visitor_options options = {})
            : writer{writer}
            , options{options} {}

        void write_direct(std::string_view value) {
            writer.write(value);
        }

        const json_visitor_options &get_options() const {
            return options;
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
            int indent;

            bool first = true;

            void write_comma() {
                if (first) {
                    first = false;
                } else {
                    instance.write_direct(",");
                    if (instance.get_options().comma_space != 0 && instance.get_options().indent == 0) {
                        instance.write_direct(std::format("{:{}}", " ", instance.get_options().comma_space));
                    }
                }
            }

            void write_indent() {
                if (instance.get_options().indent != 0) {
                    instance.write_direct(std::format("\n{:{}}", " ", indent * instance.get_options().indent));
                }
            }

        public:
            explicit array_visitor(json_visitor &instance, int indent)
                : instance{instance}
                , indent{indent}
            {
                instance.write_direct("[");
            }
        
            void write_value(auto &&value) {
                write_comma();
                write_indent();
                instance.write_value(std::forward<decltype(value)>(value));
            }

            auto begin_write_array() {
                write_comma();
                write_indent();
                return array_visitor{instance, indent + 1};
            }

            auto begin_write_object() {
                write_comma();
                write_indent();
                return object_visitor{instance, indent + 1};
            }

            void end() {
                if (!first) {
                    --indent;
                    write_indent();
                }
                instance.write_direct("]");
            }
        };
        
        class object_visitor {
        private:
            json_visitor &instance;
            int indent;

            bool first = true;

            void write_comma() {
                if (first) {
                    first = false;
                } else {
                    instance.write_direct(",");
                    if (instance.get_options().comma_space != 0 && instance.get_options().indent == 0) {
                        instance.write_direct(std::format("{:{}}", " ", instance.get_options().comma_space));
                    }
                }
            }

            void write_indent() {
                if (instance.get_options().indent != 0) {
                    instance.write_direct(std::format("\n{:{}}", " ", indent * instance.get_options().indent));
                }
            }

        public:
            explicit object_visitor(json_visitor &instance, int indent)
                : instance{instance}
                , indent{indent}
            {
                instance.write_direct("{");
            }

            void write_key(std::string_view key) {
                write_comma();
                write_indent();

                instance.write_value(key);
                instance.write_direct(":");
                if (instance.get_options().colon_space != 0) {
                    instance.write_direct(std::format("{:{}}", " ", instance.get_options().colon_space));
                }
            }
        
            void write_value(auto &&value) {
                instance.write_value(std::forward<decltype(value)>(value));
            }

            auto begin_write_array() {
                return array_visitor{instance, indent + 1};
            }

            auto begin_write_object() {
                return object_visitor{instance, indent + 1};
            }

            void end() {
                if (!first) {
                    --indent;
                    write_indent();
                }
                instance.write_direct("}");
            }
        };

    public:

        auto begin_write_array() {
            return array_visitor{*this, 1};
        }

        auto begin_write_object() {
            return object_visitor{*this, 1};
        }
    };
    
}

#endif