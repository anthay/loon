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


#include "loon_writer.h"


namespace loon {
namespace writer {
namespace {


//        ///////   //////     ///    //       
//       //     // //    //   // //   //       
//       //     // //        //   //  //       
//       //     // //       //     // //       
//       //     // //       ///////// //       
//       //     // //    // //     // //       
////////  ///////   //////  //     // //////// 

// return a usable const char pointer to the given 's'
inline const char * c_str(const std::vector<uint8_t> & s)
{
    return s.empty() ? "" : reinterpret_cast<const char *>(&s[0]);
}

// return true iff 'ch' is U+0000 .. U+001F or U+007F (ASCII control codes)
inline bool is_ctrl(uint8_t ch)
{
    return ch < 0x20 || ch == 0x7F;
}

// return ASCII hex digit representing given 'c', which MUST be in range 0..15
inline uint8_t hexchar(uint8_t c)
{
    return "0123456789ABCDEF"[c];
}

// copy 'utf8_in' to 'utf8_out' replacing control codes with their
// respective loon escape representations
void escape(std::vector<uint8_t> & utf8_out, const std::string & utf8_in)
{
    utf8_out.clear();
    utf8_out.push_back('"');

    if (!utf8_in.empty()) {
        const uint8_t * p = reinterpret_cast<const uint8_t *>(utf8_in.c_str());
        const uint8_t * const end = p + utf8_in.size();
        for (; p != end; ++p) {
            switch (*p) {
            // note: Loon does not require that {/} be escaped
            case '\\':  utf8_out.push_back('\\');   utf8_out.push_back('\\');   break;
            case '"':   utf8_out.push_back('\\');   utf8_out.push_back('\"');   break;
            case '\b':  utf8_out.push_back('\\');   utf8_out.push_back('b');    break;
            case '\f':  utf8_out.push_back('\\');   utf8_out.push_back('f');    break;
            case '\n':  utf8_out.push_back('\\');   utf8_out.push_back('n');    break;
            case '\r':  utf8_out.push_back('\\');   utf8_out.push_back('r');    break;
            case '\t':  utf8_out.push_back('\\');   utf8_out.push_back('t');    break;
            default:
                if (is_ctrl(*p)) { // => \u00XX
                    utf8_out.push_back('\\');
                    utf8_out.push_back('u');
                    utf8_out.push_back('0');
                    utf8_out.push_back('0');
                    utf8_out.push_back(hexchar(*p >> 4));
                    utf8_out.push_back(hexchar(*p & 0x0F));
                }
                else
                    utf8_out.push_back(*p);
                break;
            }
        }
    }

    utf8_out.push_back('"');
}

// write 'n' to buffer that ENDS at 'p'; return pointer to first char of output
template <typename scalar_type>
char * unsigned_to_decimal(char * p, scalar_type n)
{
    do {
        *--p = '0' + n % 10;
        n /= 10;
    }
    while (n);
    return p;
}

// write 'n' to buffer that ENDS at 'p'; return pointer to first char of output
template<typename scalar_type>
char * signed_to_decimal(char * p, scalar_type n)
{
    if (n < 0) {
        do {
            *--p = '0' - n % 10;
            n /= 10;
        }
        while (n);
        *--p = '-';
    }
    else {
        do {
            *--p = '0' + n % 10;
            n /= 10;
        }
        while (n);
    }
    return p;
}

// write 'n' to buffer that ENDS at 'p'; return pointer to first char of output
template<typename scalar_type>
char * unsigned_to_hexadecimal(char * p, scalar_type n)
{
    // output 0x00, 0x0000, 0x00000000 or 0x0000000000000000
    static_assert(CHAR_BIT == 8, "code assumes bytes are exactly 8-bits wide");
    for (int i = 0; i < sizeof(n) * 2; ++i) {
        *--p = hexchar(n & 0xF);
        n >>= 4;
    }
    while (n);
    *--p = 'x';
    *--p = '0';
    return p;
}

static const unsigned space_required = 0x00000001;

} // anonymous namespace




////////  ////////  //// //     //    ///    //////// //////// 
//     // //     //  //  //     //   // //      //    //       
//     // //     //  //  //     //  //   //     //    //       
////////  ////////   //  //     // //     //    //    //////   
//        //   //    //   //   //  /////////    //    //       
//        //    //   //    // //   //     //    //    //       
//        //     // ////    ///    //     //    //    //////// 

void base::write_indent(unsigned flags)
{
    if (pretty_) {
        if (suppress_indent_) {
            suppress_indent_ = false;
            write("  ", 2);
        }
        else {
            write(newline_.c_str(), newline_.size());
            const int num_spaces = spaces_per_indent_ * indent_;
            std::string spaces(num_spaces, ' ');
            write(spaces.c_str(), num_spaces);
        }
    }
    else if (flags & space_required) {
        write(" ", 1);
    }
}




////////  //     // ////////  //       ////  //////  
//     // //     // //     // //        //  //    // 
//     // //     // //     // //        //  //       
////////  //     // ////////  //        //  //       
//        //     // //     // //        //  //       
//        //     // //     // //        //  //    // 
//         ///////  ////////  //////// ////  //////  


// the must be overridden by user to collect writer output
void base::write(const char * utf8, int len) {}


void base::loon_arry_begin()
{
    write_indent(space_required);
    write("(arry", 5);
    empty_list_ = true;
    ++indent_;
}

void base::loon_dict_begin()
{
    write_indent(space_required);
    write("(dict", 5);
    empty_list_ = true;
    ++indent_;
}

void base::loon_arry_end()
{
    if (indent_)
        --indent_;
    if (!empty_list_)
        write_indent();
    write(")", 1);
    empty_list_ = false;
}

void base::loon_dict_end()
{
    if (indent_)
        --indent_;
    if (!empty_list_)
        write_indent();
    write(")", 1);
    empty_list_ = false;
}

void base::loon_dict_key(const std::string & value)
{
    write_indent(space_required);
    escape(buf_, value);
    write(c_str(buf_), buf_.size());
    empty_list_ = false;
    suppress_indent_ = true; // place value on same line as key
}

void base::loon_preformatted_value(const char * utf8, int len)
{
    write_indent(space_required);
    write(utf8, len);
    empty_list_ = false;
}

void base::loon_null()
{
    loon_preformatted_value("null", 4);
}

void base::loon_bool(bool value)
{
    if (value)
        loon_preformatted_value("true", 4);
    else
        loon_preformatted_value("false", 5);
}

void base::loon_dec_u32(uint32_t n)
{
    char buf[10]; // 0 .. 4294967295
    char * const end = buf + sizeof(buf);
    char * const p = unsigned_to_decimal(end, n);
    loon_preformatted_value(p, end - p);
}

void base::loon_dec_s32(int32_t n)
{
    char buf[11]; // -2147483648 .. 2147483647
    char * const end = buf + sizeof(buf);
    char * const p = signed_to_decimal(end, n);
    loon_preformatted_value(p, end - p);
}

void base::loon_hex_u32(uint32_t n)
{
    char buf[10]; // 0x0 .. 0xFFFFFFFF
    char * const end = buf + sizeof(buf);
    char * const p = unsigned_to_hexadecimal(end, n);
    loon_preformatted_value(p, end - p);
}

void base::loon_double(double n)
{
#if defined(_MSC_VER) && _MSC_VER < 1700
    const std::string s(std::to_string((long double)n));
#else
    const std::string s(std::to_string(n));
#endif
    loon_preformatted_value(s.c_str(), s.length());
}

void base::loon_string(const std::string & value)
{
    escape(buf_, value);
    loon_preformatted_value(c_str(buf_), buf_.size());
}

base::base()
: newline_("\n"), pretty_(true), empty_list_(false), suppress_indent_(false), indent_(0), spaces_per_indent_(4)
{
}

base::~base()
{
}




}} // end of namespace loon::writer
