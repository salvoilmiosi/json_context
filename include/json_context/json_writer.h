#ifndef __JSON_writer_H__
#define __JSON_writer_H__

#include <charconv>
#include <stdexcept>

#include "writer.h"

namespace json_context::writers {
    
    struct json_writer_options {
        int indent = 0;
        int colon_space = 0;
        int comma_space = 0;
    };

    template<writers::output_buffer Buffer, json_writer_options Options = json_writer_options{}>
    class json_writer {
    private:
        Buffer &buffer;

    public:
        explicit json_writer(Buffer &buffer)
            : buffer{buffer} {}

        void write_direct(std::string_view value) {
            buffer.append(value);
        }

        void write_value(std::nullptr_t) {
            write_direct("null");
        }

        template<std::integral T>
        void write_value(T value) {
            static constexpr auto buffer_size = 16;
            std::array<char, buffer_size> buf;
            auto [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), value);
            if (ec != std::errc{}) {
                throw json_writer_error{"Error writing integral value"};
            }
            write_direct(std::string_view(buf.data(), end));
        };

        template<std::floating_point T>
        void write_value(T value) {
            static constexpr auto buffer_size = 16;
            std::array<char, buffer_size> buf;
            auto [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), value);
            if (ec != std::errc{}) {
                throw json_writer_error{"Error writing floating point value"};
            }
            write_direct(std::string_view(buf.data(), end));
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
            write_direct("\"");
            write_direct(value);
            write_direct("\"");
        }

    private:
        
        class array_writer {
        private:
            json_writer &instance;
            int indent;

            bool first = true;

            void write_comma() {
                if (first) {
                    first = false;
                } else {
                    instance.write_direct(",");
                    if constexpr (Options.comma_space != 0 && Options.indent == 0) {
                        for (int i = 0; i < Options.comma_space; ++i) {
                            instance.write_direct(" ");
                        }
                    }
                }
            }

            void write_indent() {
                if constexpr (Options.indent != 0) {
                    instance.write_direct("\n");
                    for (int i = 0; i < indent * Options.indent; ++i) {
                        instance.write_direct(" ");
                    }
                }
            }

        public:
            explicit array_writer(json_writer &instance, int indent)
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
                return array_writer{instance, indent + 1};
            }

            auto begin_write_object() {
                write_comma();
                write_indent();
                return object_writer{instance, indent + 1};
            }

            void end() {
                if (!first) {
                    --indent;
                    write_indent();
                }
                instance.write_direct("]");
            }
        };
        
        class object_writer {
        private:
            json_writer &instance;
            int indent;

            bool first = true;

            void write_comma() {
                if (first) {
                    first = false;
                } else {
                    instance.write_direct(",");
                    if constexpr (Options.comma_space != 0 && Options.indent == 0) {
                        for (int i = 0; i < Options.comma_space; ++i) {
                            instance.write_direct(" ");
                        }
                    }
                }
            }

            void write_indent() {
                if constexpr (Options.indent != 0) {
                    instance.write_direct("\n");
                    for (int i = 0; i < indent * Options.indent; ++i) {
                        instance.write_direct(" ");
                    }
                }
            }

        public:
            explicit object_writer(json_writer &instance, int indent)
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
                if constexpr (Options.colon_space != 0) {
                    for (int i = 0; i < Options.colon_space; ++i) {
                        instance.write_direct(" ");
                    }
                }
            }
        
            void write_value(auto &&value) {
                instance.write_value(std::forward<decltype(value)>(value));
            }

            auto begin_write_array() {
                return array_writer{instance, indent + 1};
            }

            auto begin_write_object() {
                return object_writer{instance, indent + 1};
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
            return array_writer{*this, 1};
        }

        auto begin_write_object() {
            return object_writer{*this, 1};
        }
    };
    
}

#endif