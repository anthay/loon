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

#include "var.h"

#include <exception>



var::var()
: type_(type_undefined)
{
}

var::var(int i)
: type_(type_int), int_(i)
{
}

var::var(double i)
: type_(type_float), float_(i)
{
}

var::var(const std::string & s)
: type_(type_string), string_(s)
{
}

var var::make_arry(int size)
{
    var v;
    v.type_ = type_arry;
    v.arry_.resize(size);
    return v;
}

var var::make_dict()
{
    var v;
    v.type_ = type_dict;
    return v;
}

var var::make_bool(bool b)
{
    var v;
    v.type_ = type_bool;
    v.bool_ = b;
    return v;
}

var var::make_null()
{
    var v;
    v.type_ = type_null;
    return v;
}

void var::swap(var & v)
{
    std::swap(type_, v.type_);
    std::swap(bool_, v.bool_);
    std::swap(int_, v.int_);
    std::swap(float_, v.float_);
    std::swap(string_, v.string_);
    std::swap(arry_, v.arry_);
    std::swap(dict_, v.dict_);
}

var & var::operator=(var rhs)
{
    swap(rhs);
    return *this;
}

var & var::operator[](int i)
{
    if (type_ != type_arry)
        throw std::exception("var::operator[int] type not arry");
    return arry_[i];
}

void var::push_back(const var & v)
{
    if (type_ != type_arry)
        throw std::exception("var::push_back type not arry");
    arry_.push_back(v);
}

var & var::operator[](const std::string & s)
{
    if (type_ != type_dict)
        throw std::exception("var::operator[std::string] type not dict");
    return dict_[s];
}

bool var::key_exists(const std::string & s) const
{
    if (type_ != type_dict)
        throw std::exception("var::key_exists() type not dict");
    return dict_.find(s) != dict_.end();
}


bool var::equal(const var & v) const
{
    if (type_ != v.type_)
        return false;

    switch (type_) {
    case type_null:		return true;
    case type_bool:     return bool_ == v.bool_;
    case type_int:      return int_ == v.int_;
    case type_float:    return std::abs(float_ - v.float_) <= 1e-5 * std::abs(float_);
    case type_string:   return string_ == v.string_;
    case type_arry:     
        if (arry_ != v.arry_)
            {int i = 0;}
        return arry_ == v.arry_;
    case type_dict:
        if (dict_ != v.dict_)
            {int i = 0;}
        return dict_ == v.dict_;
    }

    throw std::exception("var::equal unknown type");
}

bool var::as_bool() const
{
    if (type_ == type_bool)
        return bool_;
    throw std::exception("var::as_bool type not type_bool");
}

int var::as_int() const
{
    if (type_ == type_int)
        return int_;
    throw std::exception("var::as_int type not type_int");
}

double var::as_float() const
{
    if (type_ == type_float)
        return float_;
    throw std::exception("var::as_float type not type_float");
}

std::string var::as_string() const
{
    if (type_ == type_string)
        return string_;
    throw std::exception("var::as_string type not type_string");
}

var::arry_t var::as_arry_t() const
{
    if (type_ == type_arry)
        return arry_;
    throw std::exception("var::as_arry_t type not type_arry");
}

var::dict_t var::as_dict_t() const
{
    if (type_ == type_dict)
        return dict_;
    throw std::exception("var::as_dict_t type not type_dict");
}
