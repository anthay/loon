#ifndef LOON_WRITER_H_INCLUDED
#define LOON_WRITER_H_INCLUDED

/*  THIS IS FREE AND UNENCUMBERED SOFTWARE RELEASED INTO THE PUBLIC DOMAIN.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    Made by Anthony C. Hay in 2014 in Wiltshire, England. See http://loonfile.info. */


#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>


namespace loon {



class writer {
public:
    writer();
    virtual ~writer();

    virtual void write(const char * utf8, int utf8_len) = 0;

    void begin_arry();
    void begin_dict();
    void end_arry();
    void end_dict();

    void dict_key(const std::string & key_name);

    void loon_value(const std::string & value);
    void loon_null();
    void loon_bool(bool value);
    void loon_dec_u32(uint32_t n);
    void loon_dec_s32(int32_t n);
    void loon_hex_u32(uint32_t n);
    void loon_string(const std::string & value);
    void loon_number(const std::string & value);

private:
    std::vector<uint8_t> buf_;
    std::string newline_;
    bool pretty_;
    bool empty_list_;
    bool suppress_indent_;
    int indent_;

    void write_newline();
    void write_indent(unsigned = 0);
};


}
#endif