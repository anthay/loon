#ifndef SAMPLE_PARSER_H_INCLUDED
#define SAMPLE_PARSER_H_INCLUDED

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


#include "loon_reader.h"
#include "var.h"

#include <stack>


// process loon text into a variant object
// this class is a demonstration of how you might use the loon::parser
class sample_parser : public loon::parser {
public:
    sample_parser() {}
    virtual ~sample_parser() {}

    const var & value() const { return v_; }

    // you call process_chunk() to feed your loon text to the parser; the
    // parser will call the corresponding virtual functions below for each
    // loon token it reads

    virtual void begin_arry();
    virtual void begin_dict();
    virtual void end_arry();
    virtual void end_dict();

    virtual void atom_null();
    virtual void atom_true();
    virtual void atom_false();
    virtual void atom_string(const std::string & value);
    virtual void atom_fixnum(const std::string & value);

private:
    var v_;
    std::stack<var> stack_;
    std::stack<std::string> key_names_;
};


var parse_loon_string(const std::string & s_utf8);


#endif
