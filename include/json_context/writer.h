#ifndef __WRITER_H__
#define __WRITER_H__

#include <string>

namespace json_context::writers {

    template<typename T>
    concept writer = requires (T obj, std::string_view data) {
        obj.write(data);
    };

    class string_writer {
    private:
        std::string &buffer;

    public:
        string_writer(std::string &buffer): buffer{buffer} {}
        
        void write(std::string_view data) {
            buffer.append(data);
        }
    };
}

#endif