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

    Made by Anthony C. Hay in 2014 in Wiltshire, England.
    See http://loonfile.info.
*/


#include "loon_reader.h"

#include <cstdint>
#include <cassert>


namespace loon {


// any occurance of this exception indicates a bug in loon::reader
loon_internal_error::loon_internal_error(const char * what)
: exception(what)
{
}

// there was a syntax error in the loon source text
loon_syntax_error::loon_syntax_error(const char * what)
: exception(what)
{
}



namespace {


// return a usable const char pointer to the given 's'
inline const char * c_str(const std::vector<uint8_t> & s)
{
    return s.empty() ? "" : reinterpret_cast<const char *>(&s[0]);
}

// return a vector containing the chars in the given null-terminated 's'
inline const std::vector<uint8_t> to_vector(const char * s)
{
    return std::vector<uint8_t>(s, s + strlen(s));
}

const std::vector<uint8_t> symname_arry(to_vector("arry"));
const std::vector<uint8_t> symname_dict(to_vector("dict"));
const std::vector<uint8_t> symname_false(to_vector("false"));
const std::vector<uint8_t> symname_null(to_vector("null"));
const std::vector<uint8_t> symname_true(to_vector("true"));



// return true iff given 'n' is first of UTF-16 surrogate pair (0xD800 <= n <= 0xDBFF)
inline bool utf16_is_surrogate_lead(uint32_t n)
{
    return (n & 0xFFFFFC00) == 0x0000D800;
}

// return true iff given 'n' is last of UTF-16 surrogate pair (0xDC00 <= n <= 0xDFFF)
inline bool utf16_is_surrogate_trail(uint32_t n)
{
    return (n & 0xFFFFFC00) == 0x0000DC00;
}

 // return the UTF-32 
inline uint32_t utf16_combine_surrogate_pair(uint32_t lead, uint32_t trail)
{
    return 0x00010000 + ((lead & 0x000003FF) << 10) + (trail & 0x000003FF);
}

inline bool is_digit(uint8_t ch)
{
    return '0' <= ch && ch <= '9';//TBD
}

inline bool is_hexdigit(uint8_t ch)
{
    return ('0' <= ch && ch <= '9')
        || ('a' <= ch && ch <= 'f')
        || ('A' <= ch && ch <= 'F');
}

inline bool is_ctrl(uint8_t ch)
{
    return ch < 0x20;
}


inline bool is_whitespace(uint8_t ch)
{
    return ch == ' ' || is_ctrl(ch);
}

inline bool is_newline(uint8_t ch)
{
    return ch == '\n' || ch == '\r';//TBD
}


// return true iff given 'c' is ASCII hex digit; return binary value of digit in 'n'
inline bool hex2bin(uint8_t c, uint32_t & n)
{
    // if this isn't ASCII, n = c - 'A' + 10 might not work
    static_assert(
        'A'==0x41 && 'B'==0x42 && 'C'==0x43 && 'D'==0x44 && 'E'==0x45 && 'F'==0x46
     && 'a'==0x61 && 'b'==0x62 && 'c'==0x63 && 'd'==0x64 && 'e'==0x65 && 'f'==0x66,
        "code assumes ASCII");

    if ('0' <= c && c <= '9')
        n = c - '0';
    else if ('A' <= c && c <= 'F')
        n = c - 'A' + 10;
    else if ('a' <= c && c <= 'f')
        n = c - 'a' + 10;
    else
        return false;

    return true;
}

// return true iff successfully read a 4 digit hex number [src, src+4) into 'n'
bool read4hex(const uint8_t * src, uint32_t & n)
{
    n = 0;
    for (int i = 0; i < 4; ++i) {
        uint32_t d;
        if (!hex2bin(src[i], d))
            return false;
        n <<= 4;
        n += d;
    }
    return true;
}




int write_utf32_as_utf8(uint8_t * dst, uint32_t n)
{
    uint8_t * p = dst;
    if (n <= 0x7f) {
        *p++ = static_cast<uint8_t>(n);
    }
    else if (n <= 0x7FF) {
        *p++ = static_cast<uint8_t>(0xC0 | (0x1f & (n >> 6)));
        *p++ = static_cast<uint8_t>(0x80 | (0x3f & n));
    } 
    else if (n <= 0xFFFF) {
        *p++ = 0xE0 | static_cast<uint8_t>((0xf & (n >> 12)));
        *p++ = 0x80 | static_cast<uint8_t>((0x3f & (n >> 6)));
        *p++ =        static_cast<uint8_t>(0x80 | (0x3f & n));
    }
    else if (n <= 0x10FFFF) {
        *p++ = static_cast<char>(0xF0 | (0x7 & (n >> 18)));
        *p++ = static_cast<char>(0x80 | (0x3f & (n >> 12)));
        *p++ = static_cast<char>(0x80 | (0x3f & (n >> 6)));
        *p++ = static_cast<char>(0x80 | (0x3f & n));
    }

    return p - dst;
}


enum expand_ret_code {
    expand_success,
    expand_incomplete_escape,
    expand_unknown_escape,
    expand_bad_utf16_escape,
    expand_bad_or_missing_utf16_surrogate_trail,
    expand_orphan_utf16_surrogate_trail };

expand_ret_code expand_loon_string_escapes(std::vector<uint8_t> & s)
{
    if (s.empty())
        return expand_success; // empty string => no escapes

    // skip straight to first escape, if any
    uint8_t * src = &s[0];
    const uint8_t * const end = src + s.size();
    while (src < end && *src != '\\')
        ++src;
    if (src == end)
        return expand_success; // string contains no escapes

    // replace all escape sequences with their respective UTF-8 expansion
    // and copy the rest of the string down over itself (because all escape
    // sequences are longer than their respective UTF-8 "expansion")
    uint8_t * dst = src;
    while (src < end) {
        ++src; // skip the \ escape character
        if (src == end)
            return expand_incomplete_escape; // got \ at very end of string

        switch (*src++) {
        // make special characters \ " and / ordinary
        case '\\':  *dst++ = '\\';  break;
        case '"':   *dst++ = '"';   break;
        case '/':   *dst++ = '/';   break; // ( / is only "special" because it's special in JSON)

        // make ordinary characters b f n r and t special
        case 'b':   *dst++ = '\b';  break;
        case 'f':   *dst++ = '\f';  break;
        case 'n':   *dst++ = '\n';  break;
        case 'r':   *dst++ = '\r';  break;
        case 't':   *dst++ = '\t';  break;

        // UTF-16 \uXXXX (or \uXXXX\uYYYY UTF-16 surrogate pair)
        case 'u':
            {
                if (end - src < 4)
                    return expand_bad_utf16_escape;
                uint32_t n, m;
                if (!read4hex(src, n))
                    return expand_bad_utf16_escape;
                src += 4;
                if (utf16_is_surrogate_lead(n)) { // => need \uYYYY trail value
                    if (end - src < 6
                        || src[0] != '\\' || src[1] != 'u'
                        || !read4hex(src+2, m)
                        || !utf16_is_surrogate_trail(m))
                        return expand_bad_or_missing_utf16_surrogate_trail;
                    n = utf16_combine_surrogate_pair(n, m);
                    src += 6;
                }
                else if (utf16_is_surrogate_trail(n))
                    return expand_orphan_utf16_surrogate_trail;
                dst += write_utf32_as_utf8(dst, n);
            }
            break;

        default:
            return expand_unknown_escape;
        }

        // copy upto next escape
        while (src < end && *src != '\\')
            *dst++ = *src++;
    }
    s.resize(dst - &s[0]); // shrink to fit

    return expand_success;
}


}





// user overrides some or all of these virtual functions to obtain
// notification of the parsed tokens
void reader_base::begin_list() {}
void reader_base::end_list() {}
void reader_base::atom_symbol(const std::vector<uint8_t> &) {}
void reader_base::atom_string(const std::vector<uint8_t> &) {}
void reader_base::atom_number(const std::vector<uint8_t> &) {}


/*  Notes about UTF-8 encoding

    - The loon reader assumes the given text is in valid UTF-8 encoding.
    - The reader does no general UTF-8 validation of the given source text.
    - If the UTF-8 BOM forms the first three bytes (0xEF 0xBB 0xBF) of the
      given loon source text it is ignored.
    - If the UTF-8 BOM appears anywhere other than the first three bytes
      of the loon source text it is parsed like any other UTF-8 character.
*/

const char * const expected_dict_or_arry = "expected 'dict' or 'arry'";
const char * const unexpected_symbol = "unexpected symbol";



void reader_base::process(uint8_t ch)
{
    bool loop;
    do {
        loop = false; // (TBD could do away with loop and just call process recursively with the unprocessed char)
        switch (state_) {
        case start:
            if (is_whitespace(ch)) {
                // ignore white space
                // remain in start state
            }
            else if (ch == ';') { // the start of a comment
                state_ = in_coment;
            }
            else if (ch == '(') { // the start of a list
                ++nest_level_;
                begin_list();
                // remain in start state
            }
            else if (ch == ')') { // the end of a list
                if (nest_level_ == 0)
                    throw loon_syntax_error("reader::process_chunk() - unbalanced ')'");
                --nest_level_;
                end_list();
                // remain in start state
            }
            else if (ch == '"') { // the start of a new string atom
                value_.clear();
                state_ = in_string;
            }
            else if (is_digit(ch)) { // {0-9} => start of integer or hex number
                value_.clear();
                value_.push_back(ch);
                state_ = num_digit_start;
            }
            else if (ch == '-' || ch == '+') { // {+-} => start of integer (possibly)
                value_.clear();
                value_.push_back(ch);
                state_ = num_sign;
            }
            else { // must be in symbol
                value_.clear();
                value_.push_back(ch);
                state_ = in_symbol;
            }
            break;

        case in_string:
            if (ch < 0x20) {
                throw loon_syntax_error("reader::process_chunk() - unescaped control character in string");
            }
            else if (ch == '"') { // the end of the string atom
                if (expand_loon_string_escapes(value_) != expand_success)
                    throw loon_syntax_error("reader::process_chunk() - bad escape sequence in string");
                atom_string(value_);
                state_ = start;
            }
            else if (ch == '\\') {
                value_.push_back(ch);
                state_ = in_string_escape;
            }
            else {
                value_.push_back(ch);
                // remain in in_string state
            }
        break;

        case in_string_escape:
            // accumulate the char following the '\', whatever it is
            value_.push_back(ch);
            state_ = in_string;
            break;

        case in_symbol:
            if (ch == '(' || ch == ')' || ch == '"' || ch == ';' || is_whitespace(ch)) {
                atom_symbol(value_);
                state_ = start;
                loop = true; // ch is unprocessed - loop round again
            }
            else {
                value_.push_back(ch);
                // remain in in_symbol state
            }
            break;

        case in_coment:
            if (is_newline(ch))
                state_ = start;
            // else remain in in_coment state
            break;

        case num_sign:
            if (is_digit(ch)) { // {+-} {0-9} => integer part
                value_.push_back(ch);
                state_ = num_digits;
            }
            else { // {+-} {ch: any character that isn't 0-9} => this was never a number
                // value_[0] is either '+' or '-' and is the start of a symbol
                loop = true; // ch is unprocessed
                state_ = in_symbol;
            }
            break;

        case num_digit_start:
            if (ch == 'x' || ch == 'X') {
                if (value_[0] == '0') { // {0} {xX} => start of hex number
                    value_.push_back(ch);
                    state_ = num_hex;
                    break;
                }
                else // {1-9} {xX} => syntax error
                    throw loon_exception(loon_exception::tbd, "loon: syntax error: incomplete hex number 0x");;
            }
            state_ = num_digits;
            // ch is unprocessed; fall through to num_digits

        case num_digits:
            if (ch == 'e' || ch == 'E') { // {0-9} {eE} => start of float exponent
                value_.push_back(ch);
                state_ = num_exp_start;
            }
            else if (ch == '.') { // {0-9} {.} => start of float fractional part
                value_.push_back(ch);
                state_ = num_frac_digits;
            }
            else if (is_digit(ch)) { // {0-9} {0-9} => more of integer part
                value_.push_back(ch);
                // remain in num_digits state
            }
            else { // {0-9} {ch: any char except e E . or 0-9} => end of number?
                atom_number(value_); // TBD do something with int value_
                loop = true; // ch is unprocessed
                state_ = start;
            }
            break;

        case num_frac_digits:
            if (ch == 'e' || ch == 'E') { // {0-9} {.} {eE} => start of float exponent
                value_.push_back(ch);
                state_ = num_exp_start;
            }
            else if (is_digit(ch)) { // {0-9} {.} {0-9} => more of fractional part
                value_.push_back(ch);
                // remain in num_frac_digits state
            }
            else { // {0-9} {.} {X: any char except e E or 0-9} => end of number?
                atom_number(value_); // TBD do something with float value_
                loop = true; // ch is unprocessed
                state_ = start;
            }
            break;

        case num_exp_start:
            if (ch == '-' || ch == '+') { // {0-9} {eE} {+-} => more exponent
                value_.push_back(ch);
                state_ = num_exp_start_digits;
            }
            else if (is_digit(ch)) { // {0-9} {eE} {0-9} => more exponent
                value_.push_back(ch);
                state_ = num_exp;
            }
            else // {0-9} {eE} {ch: any char except + - or 0-9} => syntax error
                throw loon_exception(loon_exception::tbd, "loon: syntax error: incomplete float");;
            break;

        case num_exp_start_digits:
            if (is_digit(ch)) { // {0-9} {eE} {+-} {0-9} => more exponent
                value_.push_back(ch);
                state_ = num_exp;
            }
            else // {0-9} {eE} {+-} {ch: any char except 0-9} => syntax error
                throw loon_exception(loon_exception::tbd, "loon: syntax error: incomplete float");;
            break;

        case num_exp:
            if (is_digit(ch)) { // {0-9} {eE} {+ - 0-9} {0-9} => more exponent
                value_.push_back(ch);
                // remain in num_exp
            }
            else { // {0-9} {eE} {+ - 0-9} {ch: any char except e E . or 0-9} => end of number?
                atom_number(value_); // TBD do something with float value_
                loop = true; // ch is unprocessed
                state_ = start;
            }
            break;

        case num_hex:
            if (is_hexdigit(ch)) { // {0} {xX} {0-0a-fA-F} => more hex number
                value_.push_back(ch);
                // remain in num_hex state
            }
            else if (value_.size() > 2) {
                // {0} {xX} {0-0a-fA-F} {ch: any char except 0-0a-fA-F} => end of hex number
                atom_number(value_); // TBD do something with hex value_
                loop = true; // ch is unprocessed
                state_ = start;
            }
            else // {0} {xX} {ch: any char except 0-0a-fA-F} => is an incomplete hex number
                throw loon_exception(loon_exception::tbd, "loon: syntax error: incomplete hex number");;
            break;


        default:
            throw loon_internal_error("loon: internal error: unknown state");
        }
    }
    while (loop);
}


void reader_base::process_chunk(const char * utf8, size_t utf8_len, bool is_last_chunk)
{
    const uint8_t * p = reinterpret_cast<const uint8_t *>(utf8);
    const uint8_t * const end = p + utf8_len;

    // loop once for every byte in [utf8, utf8 + utf8_len)
    for (; p != end; ++p) {
        if (*p == '\r') {
            // {CR} => newline
            ++current_line_;
            cr_ = true;
        }
        else {
            // {LF} => {newline} (don't count {LF} if preceeded by {CR}
            // because we already counted it when we saw the {CR})
            if (*p == '\n' && !cr_)
                ++current_line_;
            cr_ = false;
        }

        switch (pp_state_) { // "pre-processor" state
        case pp_in_bom_1:
            if (*p == 0xBB)
                pp_state_ = pp_in_bom_2;
            else
                throw loon_syntax_error("invalid UTF-8 BOM");
            break;

        case pp_in_bom_2:
            if (*p == 0xBF) {
                // we have silently consumed the complete UTF-8 BOM
                pp_state_ = pp_start;
            }
            else
                throw loon_syntax_error("invalid UTF-8 BOM");
            break;
    
        case pp_bom_test:
            if (*p == 0xEF) {
                pp_state_ = pp_in_bom_1;
                break;
            }
            else {
                // there is no UTF-8 BOM at the stream start
                pp_state_ = pp_start;
            }
            // fall through to pp_start

        case pp_start:
            if (*p == '\\') {
                // {\} => {unknown until we see next character}
                pp_state_ = pp_escape;
            }
            else {
                // {X: any char except \} => {X}
                process(*p);
            }
            break;

        case pp_escape:
            if (*p == '\r') {
                // {\} {CR} => {line splice}
                pp_state_ = pp_ignore_lf; // in case newlines are {CR} {LF}
            }
            else if (*p == '\n') {
                // {\} {LF} => {line splice}
                pp_state_ = pp_start;
            }
            else if (*p == '\\') {
                // {\} {\} => {\} {unknown until we see next character}
                process('\\'); // process previous \ escape
                // remain in pp_escape state
            }
            else {
                // {\} {X: any char except CR, LF and \} => {\} {X}
                process('\\');
                process(*p);
                pp_state_ = pp_start;
            }
            break;

        case pp_ignore_lf:
            if (*p == '\n') {
                // {\} {CR} {LF} => {line splice}
                pp_state_ = pp_start;
            }
            else if (*p == '\\') {
                // {\} {CR} {\} => {line splice} {unknown until we see next character}
                pp_state_ = pp_escape;
            }
            else {
                // {\} {CR} {X: any char except LF and \} => {\} {X}
                process(*p);
                pp_state_ = pp_start;
            }
            break;
        }
    }
    // we've processed all the source text we were given
    assert(p == end);

    if (is_last_chunk) {
        // we won't receive any further source text
        switch (state_) {
        case in_string:
        case in_string_escape:
            throw loon_exception(loon_exception::tbd, "loon: syntax error: incomplete string");

        case num_digit_start:
        case num_digits:
        case num_frac_digits:
        case num_exp_start:
        case num_exp_start_digits:
        case num_exp:
        case num_hex:
            // we won't be getting any more of this number
            atom_number(value_);//TBD value_ post-processing for numbers?
            state_ = start;
            break;

        case num_sign:
        case in_symbol:
            // we won't be getting any more of this symbol
            atom_symbol(value_);
            state_ = start;
            break;
        }
        if (nest_level_)
            throw loon_exception(loon_exception::tbd, "loon: syntax error: unclosed list");
    }
}




void reader_base::reset()
{
    pp_state_ = pp_bom_test;
    state_ = start;
    cr_ = false;
    current_line_ = 1;
    nest_level_ = 0;
}

reader_base::reader_base()
{
    reset();
}

reader_base::~reader_base()
{
}






// the user must override all of these virtual functions to obtain
// notification of the parsed tokens
void reader::loon_arry() {}
void reader::loon_dict() {}
void reader::loon_end() {}
void reader::loon_string(const char *, size_t) {}
void reader::loon_null() {}
void reader::loon_bool(bool) {}
void reader::loon_number(const char *, size_t) {}


// reader_base implementation calls these virtual functions
void reader::begin_list()
{
    if (at_list_start)
        throw loon_exception(loon_exception::tbd, "loon: syntax error: expected 'arry' or 'dict', got '('");
    at_list_start = true;
}

void reader::end_list()
{
    if (at_list_start)
        throw loon_exception(loon_exception::tbd, "loon: syntax error: expected 'arry' or 'dict', got ')'");
    loon_end();
}

void reader::atom_symbol(const std::vector<uint8_t> & value)
{
    if (at_list_start) {
        if (value == symname_arry)
            loon_arry();
        else if (value == symname_dict)
            loon_dict();
        else
            throw loon_exception(loon_exception::tbd, "loon: syntax error: expected 'arry' or 'dict', got other symbol");
        at_list_start = false;
    }
    else {
        if (value == symname_true)
            loon_bool(true);
        else if (value == symname_false)
            loon_bool(false);
        else if (value == symname_null)
            loon_null();
        else
            throw loon_syntax_error("unexpected symbol");
    }
}

void reader::atom_string(const std::vector<uint8_t> & value)
{
    if (at_list_start)
        throw loon_exception(loon_exception::tbd, "loon: syntax error: expected 'arry' or 'dict', got string");
    loon_string(c_str(value), value.size());
}

void reader::atom_number(const std::vector<uint8_t> & value)
{
    if (at_list_start)
        throw loon_exception(loon_exception::tbd, "loon: syntax error: expected 'arry' or 'dict', got number");
    loon_number(c_str(value), value.size());
}

void reader::reset()
{
    reader_base::reset();
    at_list_start = false;
}

reader::reader()
{
    reset();
}

reader::~reader()
{
}



}
