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


#include "sample_parser.h"

#include <sstream>



void sample_parser::begin_arry()
{
    stack_.push(var::make_arry());
}

void sample_parser::begin_dict()
{
    stack_.push(var::make_dict());
}

void sample_parser::end_arry()
{
}

void sample_parser::end_dict()
{
    switch (stack_.size()) {
        case 0:
            throw loon::loon_exception("sample_parser::end_dict loon parser error 1");
        case 1:
            // this closes the outer dict; parsing should now be complete
            // and the value of the parse is values_.top()
            if (stack_.top().type() != var::type_dict)
                throw loon::loon_exception("sample_parser::end_dict loon parser error 2");
            break;
        default:
            // this closes an inner dict value
            {
                // take the dict, d, off the top of the stack
                const var d(stack_.top());
                stack_.pop();
                if (d.type() != var::type_dict)
                    throw loon::loon_exception("sample_parser::end_dict loon parser error 3");
                // insert d into the arry or dict now at the top of the stack
                if (stack_.top().type() == var::type_arry)
                    ;//stack_.top().push_back(d);
                else if (stack_.top().type() == var::type_dict) {
                    if (key_names_.empty())
                        throw loon::loon_exception("sample_parser::end_dict loon parser error 4");
                    stack_.top().operator[](key_names_.top()) = d;
                    key_names_.pop();
                }
                else
                    throw loon::loon_exception("sample_parser::end_dict loon parser error 5");
            }
            break;
    }
}

void sample_parser::atom_null() {}
void sample_parser::atom_true() {}
void sample_parser::atom_false() {}
void sample_parser::atom_string(const std::string &) {}
void sample_parser::atom_fixnum(const std::string &) {}


var parse_loon_string(const std::string & s_utf8)
{
    sample_parser p;
    p.process_chunk(s_utf8.c_str(), s_utf8.size(), true/*is last chunk*/);
    return p.value();
}


