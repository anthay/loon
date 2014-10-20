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

    Made by Anthony Hay in 2014 in Wiltshire, England.
    See http://loonfile.info.
*/


#include <string>
#include <vector>
#include <cstdint>


namespace loon {
namespace writer {


class base {
public:
    base();
    virtual ~base();

    // The output of this class is written through this function.
    // You must override it to collect the Loon text produced.
    virtual void write(const char * utf8, int len) = 0;

    // Call this function to open a Loon arry.
    void loon_arry_begin();
    // Call this function to close a Loon arry.
    void loon_arry_end();

    // Call this function to open a Loon dict.
    void loon_dict_begin();
    // Call this function to close a Loon dict.
    void loon_dict_end();
    // Every entry in a Loon dict must have first a key, and then a value.
    // Call this function with the key name, which must be UTF-8 encoded.
    void loon_dict_key(const std::string & key_name);

    // Output the Loon value null.
    void loon_null();
    // Output the Loon value true or false.
    void loon_bool(bool value);
    // Output a Loon format decimal integer.
    void loon_dec_u32(uint32_t n);
    // Output a Loon format decimal integer.
    void loon_dec_s32(int32_t n);
    // Output a Loon format decimal integer.
    void loon_hex_u32(uint32_t n);

    // Output a Loon format decimal fraction.
    // Note: Loon does not support numeric values that cannot be represented
    // as a sequence of digits (such as Infinity and NaN).
    void loon_double(double n);

    // Output a Loon format string. The given value must be UTF-8 encoded.
    // The writer will automatically escape control characters and {\} and {"}.
    void loon_string(const std::string & value);

    // Call this function to output a pre-formatted Loon value.
    // The value is passed unchanged to the write() function.
    // Normally you would use one of the above Loon value functions
    // rather than this one. If you use this one it is your responsibility
    // to ensure that the value is in valid Loon format.
    void loon_preformatted_value(const char * utf8, int len);

    // Turn "pretty" (indented) printing on or off.
    void set_pretty(bool on) { pretty_ = on; }

    // Set the number of spaces per indentation level. (Default is 4.)
    void set_spaces_per_indent(int n) { spaces_per_indent_ = n; }

private:
    std::vector<uint8_t> buf_;
    std::string newline_;
    bool need_newline_;
    bool pretty_;
    bool empty_list_;
    bool suppress_indent_;
    int indent_;
    int spaces_per_indent_;

    void write_indent(unsigned = 0);
};


}} // end of namespace loon::writer
#endif
