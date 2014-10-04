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


#include "loon_writer.h"


namespace loon {

namespace {

std::string escaped(const std::string & s)
{
    return s; //TBD
}

}




writer::writer()
{
}

writer::~writer()
{
}

void writer::write(const char * utf8, int utf8_len)
{
}

void writer::begin_arry()
{
    write(" (arry", 6);
}

void writer::begin_dict()
{
    write(" (dict", 6);
}

void writer::end_arry()
{
    write(")", 1);
}

void writer::end_dict()
{
    write(")", 1);
}

void writer::dict_key(const std::string & value)
{
    loon_string(value);
}

void writer::loon_null()
{
    write(" null", 5);
}

void writer::loon_bool(bool value)
{
    if (value)
        write(" true", 5);
    else
        write(" false", 6);
}


void writer::loon_string(const std::string & value)
{
    std::string s(" \"");
    s += escaped(value);
    s += '"';
    write(s.c_str(), s.size());
}

void writer::loon_number(const std::string & value)
{
    write(" ", 1);
    write(value.c_str(), value.size());
}






}
