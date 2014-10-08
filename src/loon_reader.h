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

    Made by Anthony C. Hay in 2014 in Wiltshire, England. See http://loonfile.info. */


#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>


namespace  loon {


class loon_internal_error : public std::exception {
public:
    loon_internal_error(const char * what);
};

class loon_syntax_error : public std::exception {
public:
    loon_syntax_error(const char * what);
};


class loon_exception : public std::runtime_error {
public:
    enum error_id {
        tbd = 1,
        internal_error_ = 0xBADC0DE1
    };

    loon_exception(error_id id, const char * msg)
    : std::runtime_error(msg), id_(id)
    {}

    virtual error_id id() const { return id_; }

protected:
    error_id id_;
};

// xxxx: internal error: specific
// xxxx: syntax error: specific


// ignore this class: it is a loon reader implementation detail
class reader_base {

public:
    reader_base();
    virtual ~reader_base();

    virtual void reset();

    virtual void process_chunk(const char * utf8, size_t utf8_len, bool is_last_chunk);

    virtual void begin_list() = 0;
    virtual void end_list() = 0;
    virtual void atom_symbol(const std::vector<uint8_t> &) = 0;
    virtual void atom_string(const std::vector<uint8_t> &) = 0;
    virtual void atom_number(const std::vector<uint8_t> &) = 0;

    virtual int current_line() const { return current_line_; }

private:
    enum { pp_bom_test, pp_in_bom_1, pp_in_bom_2, pp_start, pp_escape, pp_ignore_lf } pp_state_;
    enum { start, in_symbol, in_string, in_string_escape, in_coment,
        num_digit_start, num_sign, num_digits, num_hex, num_exp_start,
        num_frac_digits, num_exp_start_digits, num_exp } state_;
    bool cr_;
    int current_line_;
    int nest_level_;
    std::vector<uint8_t> value_;
    void process(uint8_t ch);
};


// process loon text into a virtual function call for each loon token found;
// you should derive your own reader from this class to receive the loon data
class reader : private reader_base {

public:
    reader();
    virtual ~reader();

    virtual void reset();

    // you call process_chunk() to feed your loon text to the parser; the
    // parser will call the corresponding virtual functions below for each
    // loon token it reads
    // void process_chunk(const char * utf8, size_t utf8_len, bool is_last_chunk);
    reader_base::process_chunk; // just republish the reader_base function

    // you override these virtual functions to collect the loon data
    virtual void loon_arry() = 0;
    virtual void loon_dict() = 0;
    virtual void loon_end() = 0;
    virtual void loon_null() = 0;
    virtual void loon_bool(bool value) = 0;
    virtual void loon_string(const char * utf8, size_t utf8_len) = 0;
    virtual void loon_number(const char * utf8, size_t utf8_len) = 0;
    // Note: the string given to loon_string() and loon_number() starts
    // at utf8 and contains utf8_len UTF-8 encoded bytes, i.e. the interval
    // [utf8, utf8 + utf8_len). You must not assume the string is null
    // terminated and you must not assume that a null signals the end of
    // the string (because null is a valid UTF-8 code point and may occur
    // within a string).

private:
    bool at_list_start;

    virtual void begin_list();
    virtual void end_list();
    virtual void atom_symbol(const std::vector<uint8_t> &);
    virtual void atom_string(const std::vector<uint8_t> &);
    virtual void atom_number(const std::vector<uint8_t> &);
};



}
#endif
