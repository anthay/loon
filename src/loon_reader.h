#ifndef LOON_READER_H_INCLUDED
#define LOON_READER_H_INCLUDED

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
#include <stdexcept>
#include <cstdint>


namespace  loon {
namespace reader {


// exception error codes
enum error_id {
    no_error = 0,
    // no exception will be thrown with this id

    bad_number = 100,
    // The parser encountered a number containing invalid characters.
    // (e.g. 99abc, 9e+, are not a valid numbers.)

    bad_hex_number,
    // The parser encountered a haxadecimal number containing invalid characters.
    // (e.g. 0x9X is not a valid number.)

    dict_key_is_not_string,
    // The parser encountered a dict key that was not a string. A dict is a list of
    // zero or more key/value pairs where the value may be any object but the key must
    // be a string. For example, (dict "key" 123) is valid but (dict true 123) is not.

    incomplete_hex_number,
    // A number that started '0x' was not followed by at least one valid hexadecimal digit. (0-9a-fA-F)

    missing_arry_or_dict_symbol,
    // Something other than 'arry' or 'dict' was found immediately after an open bracket.

    missing_dict_value,
    // The dict has a key with no associated value. For example, (dict "key").

    unbalanced_close_bracket,
    // The parser encountered a close bracket for which there was no corresponding open bracket.

    unclosed_list,
    // The input text ended while there is at least one list that has not been closed. E.g. "(arry 1 2 3".

    unclosed_string,
    // The input text ended before the closing double quote in the string token.

    unescaped_control_character_in_string,
    // There is a character between U+0000 and U+001F inclusive or U+007F in the string token.
    // These are not allowed. To include shuch a character use either the UTF-16 escape (\uXXXX)
    // or other backslash escape sequences such as \n.

    unexpected_or_unknown_symbol,
    // The parser encountered a symbol it did not expect or did not recognise.
    // For example, "(arry arry)" - the second arry is unexpected.
    // For example, "(arry cake)" - cake is unknown.

    string_escape_incomplete,
    // The string ended before the backslash escape sequence was completed.

    string_escape_unknown,
    // Within a string the backslash escape was not followed by one of these characters:
    // {\} {"} {/} {b} {f} {n} {r} {t} {u}.

    bad_utf16_string_escape,
    // Within a string the backslash {u} escape was not followed by four hexadecimal digits (e.g. \u12AB).

    bad_or_missing_utf16_surrogate_trail,
    // Within a string a backslash {u} UTF-16 escape sequence was encountered that is
    // a UTF-16 surrogate lead value, but this was not followed by a valid UTF-16 surrogate
    // trail value. (A surrogate lead is in the range \uD800...\uDBFF, a surrogate trail is
    // in the range \uDC00...\uDFFF.)

    orphan_utf16_surrogate_trail,
    // Within a string a backslash {u} UTF-16 escape sequence was encountered that is
    // a UTF-16 surrogate trail value, but this was not preceded by a valid UTF-16 surrogate
    // lead value. (A surrogate lead is in the range \uD800...\uDBFF, a surrogate trail is
    // in the range \uDC00...\uDFFF.)


    // the following should never occur... the code is broken... please report to author...
    internal_error_unknown_state        = 0xBADC0DE1,
    internal_error_inconsistent_state   = 0xBADC0DE2,
};

class exception : public std::runtime_error {
public:
    exception(error_id id, int line, const char * msg)
    : std::runtime_error(msg), id_(id), line_(line)
    {}

    virtual error_id id() const { return id_; }
    virtual int line() const { return line_; }

protected:
    error_id id_;
    int line_; // Loon text line number being processed at point of exception
};


// number types for loon::reader::base::loon_number()
enum num_type {
    num_dec_int,    // -123456  decimal integer
    num_hex_int,    // 0x12ABC  hexadecimal integer, always positive
    num_float       // +9.3e-3  decimal fraction
};

// ignore this class: it is a Loon reader implementation detail
class lexer {

public:
    lexer();
    virtual ~lexer();

    virtual void reset();

    virtual void process_chunk(const char * utf8, size_t len, bool is_last_chunk);

    virtual void begin_list() = 0;
    virtual void end_list() = 0;
    virtual void atom_symbol(const std::vector<uint8_t> &) = 0;
    virtual void atom_string(const std::vector<uint8_t> &) = 0;

    virtual void atom_number(const std::vector<uint8_t> &, num_type) = 0;

    virtual int current_line() const { return current_line_; }

protected:
    int current_line_;

private:
    enum { pp_bom_test, pp_in_bom_1, pp_in_bom_2, pp_start, pp_escape, pp_ignore_lf } pp_state_;
    enum { start, in_symbol, in_string, in_string_escape, in_coment,
        num_second_digit, num_sign, num_digits, num_hex, num_exp_start,
        num_frac_digits, num_exp_start_digits, num_exp } state_;
    bool cr_;
    int nest_level_;
    std::vector<uint8_t> value_;
    void process(uint8_t ch);
};


// process Loon text into a virtual function call for each Loon token found;
// you should derive your own reader from this class to receive the Loon "events"
class base : private lexer {

public:
    base();
    virtual ~base();

    // Reset the reader to it's initial pristine state ready to start
    // processing a new Loon file.
    virtual void reset();


    /*  void process_chunk(const char * utf8, size_t len, bool is_last_chunk)

        You call process_chunk() to feed your Loon text to the parser. The
        parser will call the corresponding virtual functions below for each
        Loon token it reads.

        Notes about UTF-8 encoding
        - The Loon reader assumes the given text is in valid UTF-8 encoding.
        - The reader does no general UTF-8 validation of the given source text.
        - If the UTF-8 BOM forms the first three bytes (0xEF 0xBB 0xBF) of the
          given Loon source text it is ignored.
        - If the UTF-8 BOM appears anywhere other than the first three bytes
          of the Loon source text it is parsed like any other UTF-8 character.
    */
    using lexer::process_chunk; // just republish the lexer function

    // int current_line()
    // The reader keeps a running count of the lines as it processes them.
    // This function returns the current value of that count.
    using lexer::current_line; // just republish the lexer function


    // You must override these nine virtual functions to collect the Loon data.

    // 1. The reader encountered the start of an arry. All subsequent events
    // will be for elements of this arry, until the _corresponding_ loon_arry_end
    // event.
    virtual void loon_arry_begin() = 0;

    // 2. This closes an arry opened by the corresponding loon_arry_begin event.
    virtual void loon_arry_end() = 0;

    // 3. The reader encountered the start of a dict. All subsequent events
    // will be for elements of this dict, until the corresponding loon_dict_end
    // event.
    virtual void loon_dict_begin() = 0;

    // 4. This closes an arry opened by the corresponding loon_dict_begin event.
    virtual void loon_dict_end() = 0;

    // 5. You will get pairs of loon_dict_key/some Loon object events for
    // every entry in the dict. See loon_string() for information about the
    // parameters.
    virtual void loon_dict_key(const char * utf8, size_t len) = 0;

    // 6. The reader encountered the Loon value null.
    virtual void loon_null() = 0;

    // 7. The reader encountered either the Loon value true or false.
    virtual void loon_bool(bool value) = 0;

    // 8. The reader encountered a Loon string value.
    // The string given to loon_string() starts at utf8 and contains len
    // UTF-8 encoded bytes, i.e. the interval [utf8, utf8 + len). You must
    // not assume the string is null terminated and you must not assume
    // that a null signals the end of the string (because null is a
    // valid UTF-8 code point and may occur within a string).
    virtual void loon_string(const char * utf8, size_t len) = 0;

    // 9. The reader encountered a Loon number value.
    // [utf8, utf8 + len) is a string representing either a hex or decimal
    // integer or a decimal floating point number, as indicated by 'ntype'.
    virtual void loon_number(const char * utf8, size_t len, num_type ntype) = 0;

private:
    bool at_list_start_;

    enum list_info { arry_allow_value, dict_allow_key, dict_require_value };
    std::vector<list_info> list_state_;
    void toggle_dict_state();

    virtual void begin_list();
    virtual void end_list();
    virtual void atom_symbol(const std::vector<uint8_t> &);
    virtual void atom_string(const std::vector<uint8_t> &);
    virtual void atom_number(const std::vector<uint8_t> &, num_type);
};


}} // end on namespace loon::reader
#endif
