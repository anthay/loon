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


#include "loon_reader.h"

#include <cstdint>
#include <cassert>


namespace loon {
namespace reader {
namespace {


//        ///////   //////     ///    //       
//       //     // //    //   // //   //       
//       //     // //        //   //  //       
//       //     // //       //     // //       
//       //     // //       ///////// //       
//       //     // //    // //     // //       
////////  ///////   //////  //     // //////// 


// return {msg + 's'} as a string
std::string append(std::string msg, const std::vector<uint8_t> & s)
{
    if (s.empty())
        return msg + "''";
    return msg + "'" + std::string(&s[0], &s[0] + s.size()) + "'";
}

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

// return true iff 'ch' is U+0000 .. U+001F or U+007F (ASCII control codes)
inline bool is_ctrl(uint8_t ch)
{
    return ch < 0x20 || ch == 0x7F;
}

inline bool is_whitespace(uint8_t ch)
{
    return ch == ' ' || is_ctrl(ch);
}

inline bool is_newline(uint8_t ch)
{
    return ch == '\n' || ch == '\r';//TBD
}

bool non_symbol(uint8_t ch)
{
    return ch == '(' || ch == ')' || ch == '"' || ch == ';' || is_whitespace(ch);
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


std::string expand_exception_msg(error_id id)
{
    switch (id) {
    case string_escape_incomplete:
        return "syntax error: incomplete escape sequence in string";
    case string_escape_unknown:
        return "syntax error: unknown escape sequence in string";
    case bad_utf16_string_escape:
        return "syntax error: bad UTF-16 escape sequence in string";
    case bad_or_missing_utf16_surrogate_trail:
        return "syntax error: bad or missing UTF-16 surrogate trail escape sequence in string";
    case orphan_utf16_surrogate_trail:
        return "syntax error: orphan UTF-16 surrogate trail escape sequence in string";
    }
    return "syntax error: bad escape sequence in string";
}

error_id expand_loon_string_escapes(std::vector<uint8_t> & s)
{
    if (s.empty())
        return no_error; // empty string => no escapes

    // skip straight to first escape, if any
    uint8_t * src = &s[0];
    const uint8_t * const end = src + s.size();
    while (src < end && *src != '\\')
        ++src;
    if (src == end)
        return no_error; // string contains no escapes

    // replace all escape sequences with their respective UTF-8 expansion
    // and copy the rest of the string down over itself (because all escape
    // sequences are longer than their respective UTF-8 "expansion")
    uint8_t * dst = src;
    while (src < end) {
        ++src; // skip the \ escape character
        if (src == end)
            return string_escape_incomplete; // got \ at very end of string

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
                    return bad_utf16_string_escape;
                uint32_t n, m;
                if (!read4hex(src, n))
                    return bad_utf16_string_escape;
                src += 4;
                if (utf16_is_surrogate_lead(n)) { // => need \uYYYY trail value
                    if (end - src < 6
                        || src[0] != '\\' || src[1] != 'u'
                        || !read4hex(src+2, m)
                        || !utf16_is_surrogate_trail(m))
                        return bad_or_missing_utf16_surrogate_trail;
                    n = utf16_combine_surrogate_pair(n, m);
                    src += 6;
                }
                else if (utf16_is_surrogate_trail(n))
                    return orphan_utf16_surrogate_trail;
                dst += write_utf32_as_utf8(dst, n);
            }
            break;

        default:
            return string_escape_unknown;
        }

        // copy upto next escape
        while (src < end && *src != '\\')
            *dst++ = *src++;
    }
    s.resize(dst - &s[0]); // shrink to fit

    return no_error;
}

} // anonymous namespace



//       //////// //     // //////// ////////  
//       //        //   //  //       //     // 
//       //         // //   //       //     // 
//       //////      ///    //////   ////////  
//       //         // //   //       //   //   
//       //        //   //  //       //    //  
//////// //////// //     // //////// //     // 

// user overrides some or all of these virtual functions to obtain
// notification of the parsed tokens
void lexer::begin_list() {}
void lexer::end_list() {}
void lexer::atom_symbol(const std::vector<uint8_t> &) {}
void lexer::atom_string(const std::vector<uint8_t> &) {}
void lexer::atom_number(const std::vector<uint8_t> &, num_type) {}



/*  process() assembles the next token from the bytes it is given. As each
    token is completed process() calls the appropriate virtual function
    to signal the event to the derrived class.

    The Loon grammar can be parsed with just one character look-ahead. But
    we can't look ahaed at all - we just have to process the bytes as they
    are given to us. For example, say we are in the middle of parsing a number
    and process() is given a non-number character. At this point we can emit
    the number we have accumulated and change to the start state. We then need
    to process the non-number character we were given in the context of the
    start state. There are (at least?) three ways I could have implemented
    this: recurse (good), loop/goto (bad) or repeat the start-state code (ugly).
    I chose to recurse, i.e. process() calls itself with the non-number
    character. Note that the recursion will only ever be one level deep.
*/

void lexer::process(uint8_t ch)
{
    switch (state_) {
    case start:
        if (is_whitespace(ch) || is_ctrl(ch)) {
            // ignore white space and control characters
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
                throw exception(unbalanced_close_bracket, current_line_,
                    "syntax error: unbalanced ')'");
            --nest_level_;
            end_list();
            // remain in start state
        }
        else if (ch == '"') { // {"} => start of string
            value_.clear();
            state_ = in_string;
        }
        else if (is_digit(ch)) { // {0-9} => start of number
            value_.clear();
            value_.push_back(ch);
            state_ = num_second_digit;
        }
        else if (ch == '-' || ch == '+') { // {+-} => start of number (possibly)
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
        if (is_ctrl(ch)) {
            throw exception(unescaped_control_character_in_string, current_line_,
                "syntax error: unescaped control character in string");
        }
        else if (ch == '"') { // the end of the string atom
            const error_id id = expand_loon_string_escapes(value_);
            if (id != no_error) {
                throw exception(id, current_line_,
                    expand_exception_msg(id).c_str());
            }
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
        if (non_symbol(ch)) {
            atom_symbol(value_);
            state_ = start;
            process(ch);
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
            state_ = in_symbol;
            process(ch);
        }
        break;

    case num_second_digit:
        if (ch == 'x' || ch == 'X') {
            value_.push_back(ch);
            if (value_[0] == '0') { // {0} {xX} => start of hex number
                state_ = num_hex;
                break;
            }
            else { // {1-9} {xX} => syntax error
                throw exception(bad_number, current_line_,
                    append("syntax error: bad number ", value_).c_str());
            }
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
        else if (non_symbol(ch)) {
            // {0-9} {()"; } => end of number
            atom_number(value_, num_dec_int);
            state_ = start;
            process(ch);
        }
        else { // number merges into symbol, e.g. 99a = > bad number
            value_.push_back(ch);
            throw exception(bad_number, current_line_,
                append("syntax error: bad number ", value_).c_str());
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
        else if (non_symbol(ch)) {
            // {0-9} {.} {()"; } => end of number
            atom_number(value_, num_float);
            state_ = start;
            process(ch);
        }
        else {// got something like 9.X => bad number
            value_.push_back(ch);
            throw exception(bad_number, current_line_,
                append("syntax error: bad number ", value_).c_str());
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
        else { // {0-9} {eE} {ch: any char except + - or 0-9} => syntax error
            value_.push_back(ch);
            throw exception(bad_number, current_line_,
                append("syntax error: bad number ", value_).c_str());
        }
        break;

    case num_exp_start_digits:
        if (is_digit(ch)) { // {0-9} {eE} {+-} {0-9} => more exponent
            value_.push_back(ch);
            state_ = num_exp;
        }
        else { // {0-9} {eE} {+-} {ch: any char except 0-9} => syntax error
            value_.push_back(ch);
            throw exception(bad_number, current_line_,
                append("syntax error: bad number ", value_).c_str());
        }
        break;

    case num_exp:
        if (is_digit(ch)) { // {0-9} {eE} {+ - 0-9} {0-9} => more exponent
            value_.push_back(ch);
            // remain in num_exp
        }
        else if (non_symbol(ch)) {
            // {0-9} {eE} {+ - 0-9} {()"; } => end of number
            atom_number(value_, num_float);
            state_ = start;
            process(ch);
        }
        else { // got something like 9e9e => bad number
            value_.push_back(ch);
            throw exception(bad_number, current_line_,
                append("syntax error: bad number ", value_).c_str());
        }
        break;

    case num_hex:
        if (is_hexdigit(ch)) { // {0} {xX} {0-0a-fA-F} => more hex number
            value_.push_back(ch);
            // remain in num_hex state
        }
        else if (non_symbol(ch)) {
            // {0} {xX} {0-0a-fA-F} {()"; } => end of hex number
            if (value_.size() > 2) {
                atom_number(value_, num_hex_int);
                state_ = start;
                process(ch);
            }
            else // no hex digits following the {0} {xX} => an incomplete hex number
                throw exception(incomplete_hex_number, current_line_,
                    append("syntax error: incomplete hex number", value_).c_str());
        }
        else { // got something like 0xAX => bad hex number
            value_.push_back(ch);
            throw exception(bad_hex_number, current_line_,
                append("syntax error: bad hex number ", value_).c_str());
        }
        break;

    default:
        // all known states have been delt with above
        throw exception(internal_error_unknown_state, current_line_,
            "internal error: unknown state");
    }
}


void lexer::process_chunk(const char * utf8, size_t len, bool is_last_chunk)
{
    const uint8_t * p = reinterpret_cast<const uint8_t *>(utf8);
    const uint8_t * const end = p + len;

    // loop once for every byte in [utf8, utf8 + len)
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
            if (*p == 0xBB) // {0xEF} {0xBB} => pp_in_bom_2
                pp_state_ = pp_in_bom_2;
            else {
                // {0xEF} {anything but 0xBB} => pp_start
                process(0xEF);
                process(*p);
                pp_state_ = pp_start;
            }
            break;

        case pp_in_bom_2:
            if (*p == 0xBF) { // {0xEF} {0xBB} {0xBF} => pp_start
                // we have silently consumed the complete UTF-8 BOM
                pp_state_ = pp_start;
            }
            else {
                // {0xEF} {0xBB} {anything but 0xBF} => pp_start
                process(0xEF);
                process(0xBB);
                process(*p);
                pp_state_ = pp_start;
            }
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

        default:
            // all known states have been delt with above
            throw exception(internal_error_unknown_state, current_line_,
                "internal error: unknown state");
        }
    }
    // we've processed all the source text we were given
    assert(p == end);

    if (is_last_chunk) {
        // we won't receive any further source text
        switch (state_) {
        case in_string:
        case in_string_escape:
            throw exception(unclosed_string, current_line_,
                "syntax error: unclosed string");

        case num_second_digit:
        case num_digits:
            atom_number(value_, num_dec_int);
            break;

        case num_frac_digits:
        case num_exp_start:
        case num_exp:
            atom_number(value_, num_float);
            break;

        case num_exp_start_digits:
            throw exception(bad_number, current_line_,
                append("syntax error: bad number", value_).c_str());

        case num_hex:
            if (value_.size() > 2)
                atom_number(value_, num_hex_int);
            else
                throw exception(incomplete_hex_number, current_line_,
                    append("syntax error: incomplete hex number", value_).c_str());
            break;

        case num_sign:
        case in_symbol:
            atom_symbol(value_);
            break;
        }

        if (nest_level_)
            throw exception(unclosed_list, current_line_,
                "syntax error: unclosed list");

        state_ = start;
    }
}




void lexer::reset()
{
    pp_state_ = pp_bom_test;
    state_ = start;
    cr_ = false;
    current_line_ = 1;
    nest_level_ = 0;
}

lexer::lexer()
{
    reset();
}

lexer::~lexer()
{
}



////////  ////////  //// //     //    ///    //////// //////// 
//     // //     //  //  //     //   // //      //    //       
//     // //     //  //  //     //  //   //     //    //       
////////  ////////   //  //     // //     //    //    //////   
//        //   //    //   //   //  /////////    //    //       
//        //    //   //    // //   //     //    //    //       
//        //     // ////    ///    //     //    //    //////// 


// for non-strings update list_state_ if necessary; key -> value -> key -> value -> ...
void base::toggle_dict_state()
{
    if (!list_state_.empty()) {
        if (list_state_.back() == dict_allow_key) {
            // keys must be strings
            throw exception(dict_key_is_not_string, current_line_,
                "syntax error: dict key is not a string");
        }
        else if (list_state_.back() == dict_require_value)
            list_state_.back() = dict_allow_key;
    }
}




////////  //     // ////////  //       ////  //////  
//     // //     // //     // //        //  //    // 
//     // //     // //     // //        //  //       
////////  //     // ////////  //        //  //       
//        //     // //     // //        //  //       
//        //     // //     // //        //  //    // 
//         ///////  ////////  //////// ////  //////  


// the user must override all of these virtual functions to obtain
// notification of the parsed tokens
void base::loon_arry_begin() {}
void base::loon_arry_end() {}
void base::loon_dict_begin() {}
void base::loon_dict_end() {}
void base::loon_dict_key(const char *, size_t) {}
void base::loon_string(const char *, size_t) {}
void base::loon_null() {}
void base::loon_bool(bool) {}
void base::loon_number(const char *, size_t, num_type) {}



// lexer implementation calls these virtual functions;
// we do a little further Loon-specific processing, then pass
// them on to the user code

void base::begin_list()
{
    if (at_list_start_)
        throw exception(missing_arry_or_dict_symbol, current_line_,
            "syntax error: expected 'arry' or 'dict', got '('");
    at_list_start_ = true;
}

void base::end_list()
{
    if (at_list_start_)
        throw exception(missing_arry_or_dict_symbol, current_line_,
            "syntax error: expected 'arry' or 'dict', got ')'");
    if (list_state_.empty())
        throw exception(internal_error_inconsistent_state, current_line_,
            "internal error: inconsistent state");

    if (list_state_.back() == arry_allow_value)
        loon_arry_end();
    else if (list_state_.back() == dict_allow_key)
        loon_dict_end();
    else if (list_state_.back() == dict_require_value)
        throw exception(missing_dict_value, current_line_,
            "syntax error: dict has key with no associated value");
    else
        throw exception(internal_error_inconsistent_state, current_line_,
            "internal error: inconsistent state");

    list_state_.pop_back();
}

void base::atom_symbol(const std::vector<uint8_t> & value)
{
    toggle_dict_state();

    if (at_list_start_) {
        if (value == symname_arry) {
            list_state_.push_back(arry_allow_value);
            loon_arry_begin();
        }
        else if (value == symname_dict) {
            list_state_.push_back(dict_allow_key);
            loon_dict_begin();
        }
        else
            throw exception(missing_arry_or_dict_symbol, current_line_,
                append("syntax error: expected 'arry' or 'dict', got ", value).c_str());
        at_list_start_ = false;
    }
    else {
        if (value == symname_true)
            loon_bool(true);
        else if (value == symname_false)
            loon_bool(false);
        else if (value == symname_null)
            loon_null();
        else
            throw exception(unexpected_or_unknown_symbol, current_line_,
                append("syntax error: unexpected or unknown symbol ", value).c_str());
    }
}

void base::atom_string(const std::vector<uint8_t> & value)
{
    if (at_list_start_)
        throw exception(missing_arry_or_dict_symbol, current_line_,
            append("syntax error: expected 'arry' or 'dict', got ", value).c_str());

    if (list_state_.empty())
        loon_string(c_str(value), value.size());
    else {
        if (list_state_.back() == dict_allow_key) {
            loon_dict_key(c_str(value), value.size());
            list_state_.back() = dict_require_value;
        }
        else {
            loon_string(c_str(value), value.size());
            if (list_state_.back() == dict_require_value)
                list_state_.back() = dict_allow_key;
        }
    }
}

void base::atom_number(const std::vector<uint8_t> & value, num_type ntype)
{
    if (at_list_start_)
        throw exception(missing_arry_or_dict_symbol, current_line_,
            append("syntax error: expected 'arry' or 'dict', got ", value).c_str());

    toggle_dict_state();
    loon_number(c_str(value), value.size(), ntype);
}

void base::reset()
{
    lexer::reset();
    at_list_start_ = false;
    list_state_.clear();
}

base::base()
{
    reset();
}

base::~base()
{
}



}} // end of namespace loon::reader
