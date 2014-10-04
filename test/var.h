#ifndef VAR_H_INCLUDED
#define VAR_H_INCLUDED

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


#include <vector>
#include <map>
#include <string>


// a simple variant type
// (the requirement is for a simple, correct type with sufficient functionality
// to excercisse loon and with dependencies on only vanilla C++ libraries (e.g.
// no boost); space and speed efficiencies are not part of this requirement) 
class var {
public:
    typedef enum { type_undefined, type_null, type_bool, type_int, type_string, type_arry, type_dict } variant_type;
    typedef std::vector<var> arry_t;
    typedef std::map<std::string, var> dict_t;

    var();
    explicit var(int);
    explicit var(const std::string &);
    static var make_arry(int size = 0);
    static var make_dict();
    static var make_bool(bool);
    static var make_null();

    void swap(var &);

    var & operator=(var rhs);
    var & operator[](int); // for access to type_arry objects only
    var & operator[](const std::string &); // for access to type_dict objects only
    void push_back(const var &); // for appending to type_arry objects only

    variant_type type() const { return type_; }
    bool equal(const var & rhs) const;

    bool as_bool() const;
    int as_int() const;
    std::string as_string() const;
    arry_t as_arry_t() const;
    dict_t as_dict_t() const;

private:
    variant_type type_;
    bool bool_;
    int int_;
    std::string string_;
    arry_t arry_;
    dict_t dict_;
};

inline bool operator==(const var & lhs, const var & rhs)
{
    return lhs.equal(rhs);
}


#endif
