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


/*  loon-cpp V0.01

    This is intended to be both an example of loon reader and writer
    usage and a sufficiently comprehensive unit test to some confidence
    that the implementation is correct.
    
    This module is divided into three parts:

        - direct_usage_example
        - variant_usage_example
        - unit_test
*/


#include "loon_reader.h"
#include "loon_writer.h"

#include "var.h" // a sample variant class used for testing, not part of loon itself

#include <stack>
#include <iostream>
#include <sstream>


unsigned g_test_count;      // count of number of unit tests executed
unsigned g_fault_count;     // count of number of unit tests that fail

// write a message to std::cout if value == expected_value is false
#define TEST_EQUAL(value, expected_value)               \
{                                                       \
    ++g_test_count;                                     \
    if (!((value) == (expected_value))) {               \
        std::cout                                       \
            << __FILE__ << '(' << __LINE__ << ") : "    \
            << "in '" << __FUNCTION__ << "'"            \
            << " TEST_EQUAL failed\n";                  \
        ++g_fault_count;                                \
    }                                                   \
}

#define TEST_EXCEPTION(expression, exception_expected)  \
    {                                                   \
        bool got_exception = false;                     \
        try {                                           \
            expression;                                 \
        }                                               \
        catch (const exception_expected &) {            \
            got_exception = true;                       \
        }                                               \
        TEST_EQUAL(got_exception, true);                \
    }                                                   \





////////  //// ////////  ////////  //////  //////// 
//     //  //  //     // //       //    //    //    
//     //  //  //     // //       //          //    
//     //  //  ////////  //////   //          //    
//     //  //  //   //   //       //          //    
//     //  //  //    //  //       //    //    //    
////////  //// //     // ////////  //////     //   

// an example of serialising/unserialising directly from/to a specific
// C++ structure (not via an intermediate general variant structure)
namespace direct_usage_example {


// say we have this structure we wish to persist via loon
struct library {
    std::string name;

    struct book {
        std::string name;
        std::string author;
        book(const std::string & name, const std::string & author)
            : name(name), author(author) {}
        book() {}
        void clear() { name.clear(); author.clear(); }
    };
    std::vector<book> books;
};

bool operator==(const library::book & lhs, const library::book & rhs)
{
    return lhs.name == rhs.name
        && lhs.author == rhs.author;
}

bool operator==(const library & lhs, const library & rhs)
{
    return lhs.name == rhs.name
        && lhs.books == rhs.books;
}


// create a string of loon text representing the given structure 'lib'
std::string serialise(const library & lib)
{
    // to use the loon::writer you derive your own writer class
    // from it and provide an implementation for the write() function
    struct direct_writer : public loon::writer {
        std::string str;

        virtual void write(const char * utf8, int utf8_len)
        {
            // this is the loon::serialiser output; write it to a file
            // or whatever you want - here we just append it to a string
            str += std::string(utf8, utf8_len);
        }
    };

    // then you use your writer to record the structure of the object
    // you want to serialise, like this...
    direct_writer writer;

    // the obvious way to represent a struct in loon is with a dict
    writer.begin_dict();
    {
        // (it's not necessary to use {blocks}, but they help keep track
        // of the arry and dict structures)

        // the library object has a name field with a string value
        writer.dict_key("name");
        writer.loon_string(lib.name);

        // the library object has vector of books; the most obvious way
        // to serialise that will be as an arry
        writer.dict_key("books");
        writer.begin_arry();
        {
            // serialise each book in the vector
            typedef std::vector<library::book>::const_iterator iter;
            for (iter i = lib.books.begin(); i != lib.books.end(); ++i) {
                // each book is probably best represented as a dict
                writer.begin_dict();
                {
                    writer.dict_key("author");
                    writer.loon_string(i->author);

                    writer.dict_key("name");
                    writer.loon_string(i->name);
                }
                writer.end_dict();
            }
        }
        // close the arry of books
        writer.end_arry();
    }
    // finally, close the top level dict
    writer.end_dict();

    // our sample_writer accumulated all the loon::writer
    // output in its own string, so we just return that
    return writer.str;
}


// create a library object from the given loon text
library unserialise(const std::string & loon_text_utf8)
{
    // to use the loon::reader you derive your own reader class
    // from it and provide an implementation for the "events"
    class direct_reader : private loon::reader {
    public:
        direct_reader() : state_(start) {}

        // feed the loon text to be parsed to this function
        void process_chunk(const char * utf8, size_t utf8_len, bool is_last_chunk)
        {
            reader::process_chunk(utf8, utf8_len, is_last_chunk);
        }

        // return the final result of the parse
        const library & result() const
        {
            if (state_ == finish)
                return lib_;
            throw std::exception("sample_reader: incomplete");
        }

    private:
        /*  We'll use a simple state machine to help keep track of things.
            It might help to see a sample of the loon text we'll be reading:

            (dict
                "name" "The British Library"
                "books" (arry
                    (dict
                        "author" "Dr. Seuss"
                        "name" "Green Eggs and Ham")
                    (dict
                        "author" "Douglas Hofstadter"
                        "name" "Gödel, Escher, Bach")
                )
            )
        */
        enum {
            start,      // expecting a dict (->lib)
            lib,        // expecting library name (->lib_name) or books arry (->books) or end_dict (->finish)
            lib_name,   // expecting a string (->lib)
            books,      // expecting a books arry (->in_books)
            in_books,   // expecting a book dict (->book) or end_arry (->lib)
            book,       // expecting a book name (->book_name) or author (->book_auth) or end_dict (->in_books)
            book_name,  // expecting a string (->book)
            book_auth,  // expecting a string (->book)
            finish      // expecting nothing further
        } state_;
        /*  Note 1: We'll just accept that we'll not notice duplicate values.
            E.g. you can go from lib -> lib_name -> lib more than once and it
            will not be detected as an error.

            Note 2: OK, it's not as simple as I'd like, in fact it's rather
            tedious and error-prone, but at least the end result will be the
            loon data read directly into our target data structure, which is
            potentially fast and memory efficient. If you don't want to
            derive a loon::reader for every data structure you could just
            create one reader to read the loon data into a variant, which
            you would then have to query as necessary. An example of this
            approach is shown in variant_usage_example below.
        */

        // we'll also need to accumulate the data somewhere...
        library lib_;
        library::book b_;

        virtual void loon_dict()
        {
            if (state_ == start)
                state_ = lib;
            else if (state_ == in_books) {
                b_.clear();
                state_ = book;
            }
            else
                throw std::exception("sample_reader: unexpected 'begin_dict'");
        }

        virtual void loon_arry()
        {
            if (state_ == books)
                state_ = in_books;
            else
                throw std::exception("sample_reader: unexpected 'begin_arry'");
        }

        virtual void loon_end()
        {
            if (state_ == in_books)
                state_ = lib;
            else if (state_ == lib)
                state_ = finish;
            else if (state_ == book) {
                lib_.books.push_back(b_);
                state_ = in_books;
            }
            else
                throw std::exception("sample_reader: unexpected ')'");
        }

        virtual void loon_string(const char * value_utf8, size_t value_utf8_len)
        {
            const std::string value(value_utf8, value_utf8+value_utf8_len);
            switch (state_) {
            case lib:
                if (value == "name")
                    state_ = lib_name;
                else if (value == "books")
                    state_ = books;
                else
                    throw std::exception("sample_reader: unexpected 'loon_string'");
                break;
            case book:
                if (value == "name")
                    state_ = book_name;
                else if (value == "author")
                    state_ = book_auth;
                else
                    throw std::exception("sample_reader: unexpected string");
                break;
            case lib_name:
                lib_.name = value;
                state_ = lib;
                break;
            case book_name:
                b_.name = value;
                state_ = book;
                break;
            case book_auth:
                b_.author = value;
                state_ = book;
                break;
            default:
                throw std::exception("direct_reader: unexpected string");
            }
        }

        // for our library object the occurance of any of these events would be wrong
        virtual void loon_null() { throw std::exception("sample_reader: unexpected null"); }
        virtual void loon_bool(bool) { throw std::exception("sample_reader: unexpected bool"); }
        virtual void loon_number(const char *, size_t) { throw std::exception("sample_reader: unexpected number"); }
    };

    // because we were given the complete loon source text in one string we
    // can feed it all to our parser in one chunk; if we were reading it from
    // a file we could feed it in one arbitrary sized buffer full after
    // another until it was all processed
    direct_reader reader;
    reader.process_chunk(loon_text_utf8.c_str(), loon_text_utf8.size(), true/*is last chunk*/);

    // our sample_reader accumulated all the loon::reader
    // events in its own library object, so we just return that
    return reader.result();
}


void test()
{
    // let's make an object and populate it with some data
    library a;
    a.name = "The British Library";
    a.books.push_back(library::book("Green Eggs and Ham", "Dr. Seuss"));
    a.books.push_back(library::book("G\xC3\xB6""del, Escher, Bach", "Douglas Hofstadter"));

    // if we serialise our object we'd expect the loon text to look like this
    const char * expected = {
#if 1
        "\n"
        "(dict\n"
        "    \"name\"  \"The British Library\"\n"
        "    \"books\"  (arry\n"
        "        (dict\n"
        "            \"author\"  \"Dr. Seuss\"\n"
        "            \"name\"  \"Green Eggs and Ham\"\n"
        "        )\n"
        "        (dict\n"
        "            \"author\"  \"Douglas Hofstadter\"\n"
        "            \"name\"  \"G\xC3\xB6""del, Escher, Bach\"\n"
        "        )\n"
        "    )\n"
        ")"
#else
        " (dict \"name\" \"The British Library\""
        " \"books\" (arry (dict \"author\" \"Dr. Seuss\" \"name\" \"Green Eggs and Ham\")"
        " (dict \"author\" \"Douglas Hofstadter\" \"name\" \"G\xC3\xB6""del, Escher, Bach\")))"
#endif
    };
    const std::string serialised_a(serialise(a));
    TEST_EQUAL(serialised_a, expected);

    // if we unserialise the loon text we just created we'd expect to
    // get back an object containing the same data as the original
    const library b(unserialise(serialised_a));
    TEST_EQUAL(b, a);
}

}




//     //    ///    ////////  ////    ///    //    // //////// 
//     //   // //   //     //  //    // //   ///   //    //    
//     //  //   //  //     //  //   //   //  ////  //    //    
//     // //     // ////////   //  //     // // // //    //    
 //   //  ///////// //   //    //  ///////// //  ////    //    
  // //   //     // //    //   //  //     // //   ///    //    
   ///    //     // //     // //// //     // //    //    //    

namespace variant_usage_example {

// an example of serialising/unserialising a variant C++ structure
// that excersises all supported loon datatypes


// to use the loon::reader you derive your own reader class
// from it and provide an implementation for the "events"
class variant_reader : private loon::reader {
public:
    variant_reader() {}

    // feed the loon text to be parsed to this function
    void process_chunk(const char * utf8, size_t utf8_len, bool is_last_chunk)
    {
        reader::process_chunk(utf8, utf8_len, is_last_chunk);
    }

    // return the final result of the parse
    const var & result() const
    {
        if (stack_.size() == 1)
            return stack_.top().value;
        throw std::exception("variant_reader: incomplete");
    }

private:
    struct node {
        var value;
        std::string key;
        bool have_key;

        node() : have_key(false) {}
        node(const var & v) : value(v), have_key(false) {}
    };
    std::stack<node> stack_;


    virtual void loon_arry()
    {
        stack_.push(node(var::make_arry()));
    }

    virtual void loon_dict()
    {
        stack_.push(node(var::make_dict()));
    }

    virtual void loon_end()
    {
        switch (stack_.size()) {
        case 0:
            // loon::reader will never give a loon_end event with no
            // prior loon_dict or loon_arry event
            throw loon::loon_internal_error("variant_reader: internal error");
        case 1:
            // this closes the outer dict or arry; parsing should now be complete
            // and the value of the parse is stack_.top()
            break;
        default:
            // this closes an inner dict or arry
            {
                // get the newly completed node n
                const node n(stack_.top()); stack_.pop();

                // insert n.value into the arry or dict now at the top of the stack
                node & top = stack_.top();
                if (top.value.type() == var::type_arry)
                    top.value.push_back(n.value);
                else if (top.value.type() == var::type_dict) {
                    if (top.have_key) {
                        top.value[top.key] = n.value;
                        top.have_key = false;
                    }
                    else
                        throw loon::loon_syntax_error("dict: missing value");
                }
                else
                    throw loon::loon_internal_error("variant_reader: internal error");
            }
            break;
        }
    }

    virtual void loon_string(const char * value_utf8, size_t value_utf8_len)
    {
        const std::string value(value_utf8, value_utf8+value_utf8_len);
        if (stack_.empty())
            stack_.push(node(var(value)));
        else {
            node & top = stack_.top();
            if (top.value.type() == var::type_arry)
                top.value.push_back(var(value)); // given string is arry value
            else if (top.value.type() == var::type_dict) {
                if (top.have_key)
                    top.value[top.key] = var(value); // given string is the dict value
                else
                    top.key = value; // given string is the dict key
                top.have_key = !top.have_key;
            }
            else
                throw loon::loon_syntax_error("unexpected token");
        }
    }

    virtual void loon_number(const char * value_utf8, size_t value_utf8_len)
    {
        const std::string value(value_utf8, value_utf8+value_utf8_len);
        int i(atoi(value.c_str()));//TBD
        new_leaf(var(i));
    }

    virtual void loon_bool(bool value)
    {
        new_leaf(var::make_bool(value));
    }

    virtual void loon_null()
    {
        new_leaf(var::make_null());
    }

    void new_leaf(const var & value)
    {
        if (stack_.empty())
            stack_.push(node(value));
        else {
            node & top = stack_.top();
            if (top.value.type() == var::type_arry)
                top.value.push_back(value);
            else if (top.value.type() == var::type_dict) {
                if (top.have_key) {
                    top.value[top.key] = value;
                    top.have_key = false;
                }
                else
                    throw loon::loon_syntax_error("dict key must be a string");
            }
            else
                throw loon::loon_syntax_error("unexpected token");
        }
    }
};


// create a library object from the given loon text
var unserialise(const std::string & loon_text_utf8)
{
    // because we were given the complete loon source text in one string we
    // can feed it all to our parser in one chunk; if we were reading it from
    // a file we could feed it in one arbitrary sized buffer full after
    // another until it was all processed
    variant_reader reader;
    reader.process_chunk(loon_text_utf8.c_str(), loon_text_utf8.size(), true/*is last chunk*/);

    // our sample_reader accumulated all the loon::reader
    // events in its own var object, so we just return that
    return reader.result();
}


// to use the loon::writer you derive your own writer class
// from it and provide an implementation for the write() function
struct variant_writer : public loon::writer {
    std::string str;

    virtual void write(const char * utf8, int utf8_len)
    {
        // this is the loon::serialiser output; write it to a file
        // or whatever you want - here we just append it to a string
        str += std::string(utf8, utf8_len);
    }
};


// create a string of loon text representing the given variant 'v'
std::string serialise(const var & v, variant_writer & writer)
{
    switch (v.type()) {
    case var::type_null:    writer.loon_null(); break;
    case var::type_bool:    writer.loon_bool(v.as_bool()); break;
    case var::type_string:  writer.loon_string(v.as_string()); break;

    case var::type_int:
        {
            std::ostringstream oss;
            oss << v.as_int();
            writer.loon_number(oss.str());
        }
        break;

    case var::type_arry:
        {
            writer.begin_arry();
            const var::arry_t & a(v.as_arry_t());
            if (!a.empty()) {
                for (var::arry_t::const_iterator i = a.begin(); i != a.end(); ++i) {
                    serialise(*i, writer);
                }
            }
            writer.end_arry();
        }
        break;

    case var::type_dict:
        {
            writer.begin_dict();
            const var::dict_t & a(v.as_dict_t());
            if (!a.empty()) {
                for (var::dict_t::const_iterator i = a.begin(); i != a.end(); ++i) {
                    writer.dict_key(i->first);
                    serialise(i->second, writer);
                }
            }
            writer.end_dict();
        }
        break;

    default:
        throw std::exception("serialise unknown type");
    }

    // our sample_writer accumulated all the loon::writer
    // output in its own string, so we just return that
    return writer.str;
}

// create a string of loon text representing the given variant 'v'
std::string serialise(const var & v)
{
    // then you use your writer to record the structure of the object
    // you want to serialise, like this...
    variant_writer writer;

    serialise(v, writer);

    // our sample_writer accumulated all the loon::writer
    // output in its own string, so we just return that
    return writer.str;
}



void test()
{
    const char * loon_text = {

        "; ### a self-explanitory loon sample file ###\n"
        "\n"
        "; - loon is a data serialisation file format based on S-expressions\n"
        "; - loon is designed to be easy for both people and machines to read\n"
        "; - loon files are required to be UTF-8 encoded and may have a UTF-8 BOM\n"
        "\n"
        "; this is a comment - it starts with a semicolon and ends at the line end\n"
        "\n"
        "; a loon file contains a single value; in this sample that value is a dict\n"
        "; structure, which itself may contain many values\n"
        "(dict\n"
        "    ; a dict is an unordered list of zero or more key/value pairs enclosed\n"
        "    ; between \"(dict\" and \")\" tokens;  the key is always a string enclosed\n"
        "    ; in double quotes and must be unique in the dict\n"
        "    \"key\" \"value\"\n"
        "\n"
        "    ; the value must be one of these three simple types, or a nested\n"
        "    ; arry or dict structure or null\n"
        "    \"a number\" 1234\n"
        "    \"a boolean\" false\n"
        "    \"a string\" \"any Unicode text except backslash and double quote\"\n"
        "    \"a nothing (has no type or value)\" null\n"
        "\n"
        "    ; an arry is an ordered list of zero or more values enclosed between\n"
        "    ; \"(arry\" and \")\" tokens; the values do not have to be of the same type\n"
        "    \"heterogeneous array\" (arry \"the\" 1 true \"brace style\")\n"
        "\n"
        "    \"an array of arrays\" (arry\n"
        "        (arry 1 0 0)\n"
        "        (arry 0 1 0)\n"
        "        (arry 0 0 1)\n"
        "    )\n"
        "\n"
        "    \"books\" (arry\n"
        "        (dict\n"
        "            \"name\" \"Green Eggs and Ham\"\n"
        "            \"author\" \"Dr. Seuss\")\n"
        "        (dict\n"
        "            \"name\" \"G\xC3\xB6""del, Escher, Bach\"\n"
        "            \"author\" \"Douglas Hofstadter\")\n"
        "    )\n"
        "\n"
        "    \"an empty arry\" (arry)\n"
        "    \"an empty dict\" (dict)\n"
        "\n"
        "    ; white space is needed only where necessary to separate symbols that\n"
        "    ; would otherwise merge and for human readability\n"
        "    ; these three key/value pairs have the minimum necessary white space\n"
        "\"one two\"(arry 1 2)\"twelve\"(arry 12)\"three\"3\n"
        "\n"
        "    ; bad loon: examples of ill-formed expressions\n"
        "    ; (dict \"key\" 0 \"key\" 1) - keys not unique\n"
        "    ; (dict \"key\")           - missing value\n"
        "    ; (dict 0 \"zero\")        - key not a string\n"
        "    ; (dict \"key\" hat)       - hat is not a valid loon value\n"
        "    ; (dict \"key\" arry)      - arry doesn't make sense in this context\n"
        "\n"
        "    ; --- and that's all there is to a loon file ---\n"
        "\n"
        "    \"loon\" (arry\n"
        "        \"a foolish fellow?\"\n"
        "        \"list oriented object notation?\"\n"
        "        \"JSON done with S-expressions?\"\n"
        "    )\n"
        ")\n"

    };
    
    var expected(var::make_dict());
    expected["key"] = var("value");
    expected["a number"] = var(1234);
    expected["a boolean"] = var::make_bool(false);
    expected["a string"] = var("any Unicode text except backslash and double quote");
    expected["a nothing (has no type or value)"] = var::make_null();

    expected["heterogeneous array"] = var::make_arry();
    expected["heterogeneous array"].push_back(var("the"));
    expected["heterogeneous array"].push_back(var(1));
    expected["heterogeneous array"].push_back(var::make_bool(true));
    expected["heterogeneous array"].push_back(var("brace style"));
    
    expected["an array of arrays"] = var::make_arry();
    expected["an array of arrays"].push_back(var::make_arry());
    expected["an array of arrays"].push_back(var::make_arry());
    expected["an array of arrays"].push_back(var::make_arry());
    expected["an array of arrays"][0].push_back(var(1));
    expected["an array of arrays"][0].push_back(var(0));
    expected["an array of arrays"][0].push_back(var(0));
    expected["an array of arrays"][1].push_back(var(0));
    expected["an array of arrays"][1].push_back(var(1));
    expected["an array of arrays"][1].push_back(var(0));
    expected["an array of arrays"][2].push_back(var(0));
    expected["an array of arrays"][2].push_back(var(0));
    expected["an array of arrays"][2].push_back(var(1));

    expected["books"] = var::make_arry();
    expected["books"].push_back(var::make_dict());
    expected["books"].push_back(var::make_dict());
    expected["books"][0]["name"] = var("Green Eggs and Ham");
    expected["books"][0]["author"] = var("Dr. Seuss");
    expected["books"][1]["name"] = var("G\xC3\xB6""del, Escher, Bach");
    expected["books"][1]["author"] = var("Douglas Hofstadter");

    expected["an empty arry"] = var::make_arry();
    expected["an empty dict"] = var::make_dict();

    expected["one two"] = var::make_arry(2);
    expected["one two"][0] = var(1);
    expected["one two"][1] = var(2);

    expected["twelve"] = var::make_arry(1);
    expected["twelve"][0] = var(12);

    expected["three"] = var(3);

    expected["loon"] = var::make_arry(3);
    expected["loon"][0] = var("a foolish fellow?");
    expected["loon"][1] = var("list oriented object notation?");
    expected["loon"][2] = var("JSON done with S-expressions?");

    const var a(unserialise(loon_text));
    TEST_EQUAL(a, expected);


    const char * expected_loon = {
#if 0
        " (dict \"a boolean\" false \"a nothing (has no type or value)\" null"
        " \"a number\" 1234 \"a string\" \"any Unicode text except backslash and double quote\""
        " \"an array of arrays\" (arry (arry 1 0 0) (arry 0 1 0) (arry 0 0 1))"
        " \"an empty arry\" (arry) \"an empty dict\" (dict)"
        " \"books\" (arry (dict \"author\" \"Dr. Seuss\" \"name\" \"Green Eggs and Ham\")"
        " (dict \"author\" \"Douglas Hofstadter\" \"name\" \"G\xC3\xB6""del, Escher, Bach\"))"
        " \"heterogeneous array\" (arry \"the\" 1 true \"brace style\") \"key\" \"value\""
        " \"loon\" (arry \"a foolish fellow?\" \"list oriented object notation?\" \"JSON done with S-expressions?\")"
        " \"one two\" (arry 1 2) \"three\" 3 \"twelve\" (arry 12))"
#else
        "\n"
        "(dict\n"
        "    \"a boolean\"  false\n"
        "    \"a nothing (has no type or value)\"  null\n"
        "    \"a number\"  1234\n"
        "    \"a string\"  \"any Unicode text except backslash and double quote\"\n"
        "    \"an array of arrays\"  (arry\n"
        "        (arry\n"
        "            1\n"
        "            0\n"
        "            0\n"
        "        )\n"
        "        (arry\n"
        "            0\n"
        "            1\n"
        "            0\n"
        "        )\n"
        "        (arry\n"
        "            0\n"
        "            0\n"
        "            1\n"
        "        )\n"
        "    )\n"
        "    \"an empty arry\"  (arry)\n"
        "    \"an empty dict\"  (dict)\n"
        "    \"books\"  (arry\n"
        "        (dict\n"
        "            \"author\"  \"Dr. Seuss\"\n"
        "            \"name\"  \"Green Eggs and Ham\"\n"
        "        )\n"
        "        (dict\n"
        "            \"author\"  \"Douglas Hofstadter\"\n"
        "            \"name\"  \"G\xC3\xB6""del, Escher, Bach\"\n"
        "        )\n"
        "    )\n"
        "    \"heterogeneous array\"  (arry\n"
        "        \"the\"\n"
        "        1\n"
        "        true\n"
        "        \"brace style\"\n"
        "    )\n"
        "    \"key\"  \"value\"\n"
        "    \"loon\"  (arry\n"
        "        \"a foolish fellow?\"\n"
        "        \"list oriented object notation?\"\n"
        "        \"JSON done with S-expressions?\"\n"
        "    )\n"
        "    \"one two\"  (arry\n"
        "        1\n"
        "        2\n"
        "    )\n"
        "    \"three\"  3\n"
        "    \"twelve\"  (arry\n"
        "        12\n"
        "    )\n"
        ")"
#endif
    };

    const std::string b(serialise(a));
    TEST_EQUAL(b, expected_loon);
}

}




//     // //    // //// ////////    //////// ////////  //////  //////// 
//     // ///   //  //     //          //    //       //    //    //    
//     // ////  //  //     //          //    //       //          //    
//     // // // //  //     //          //    //////    //////     //    
//     // //  ////  //     //          //    //             //    //    
//     // //   ///  //     //          //    //       //    //    //    
 ///////  //    // ////    //          //    ////////  //////     //    

namespace unit_test {

// excersise the loon parser and serialiser to flush out bugs

// we'll be using the variant_usage_example::variant_reader for these tests
using variant_usage_example::variant_reader;
using variant_usage_example::serialise;

// return given 's' with {\} {LF}, {\} {CR} and {\} {CR} {LF} line splice
// sequences inserted before each and every character
std::string insert_line_continuations(const std::string & s)
{
    std::string result;
    for (size_t i = 0; i < s.size(); ++i) {
        result += "\\\n";
        result += "\\\r";
        result += "\\\r\n";
        result += s[i];
    }
    return result;
}

// feed all given 'loon_text' to the reader in one chunk
var unserialise(const std::string & loon_text)
{
    variant_reader reader;
    reader.process_chunk(loon_text.c_str(), loon_text.size(), true/*is last chunk*/);
    return reader.result();
}

// feed the given 'loon_text' to the reader 'chunk_size' bytes at a time
var unserialise(const std::string & loon_text, int chunk_size)
{
    variant_reader reader;
    const char * p = loon_text.c_str();
    const char * p_end = p + loon_text.size();
    while (p < p_end) {
        // test that empty chunks do no harm
        reader.process_chunk(0, 0, false/*not last chunk*/);
        if (p_end - p < chunk_size)
            chunk_size = p_end - p;
        reader.process_chunk(p, chunk_size, false/*not last chunk*/);
        p += chunk_size;
    }
    reader.process_chunk(0, 0, false/*not last chunk*/); // another nop test
    reader.process_chunk(0, 0, true/*is last chunk*/);
    return reader.result();
}

// feed the given 'loon_text' to the unserialiser and test that the output is
// the given 'expected'; repeat the test with varying chunk sizes
// WARNING: O(n^2) execution time, n = loon_text.size()
void test2(const std::string & loon_text, const var & expected)
{
    // test lots of different chunk sizes
    for (int chunk_size = 1; chunk_size <= (int)loon_text.size(); ++chunk_size)
        TEST_EQUAL(unserialise(loon_text, chunk_size), expected);

    // test it also works when parsed in one big chunk
    TEST_EQUAL(unserialise(loon_text), expected);
}

// feed the given 'loon_text' to the unserialiser, excersising
// BOM and line continuations
void test(const std::string & loon_text, const var & expected)
{
    test2(loon_text, expected);

    // do it all again but this time preceeded by a UTF-8 BOM
    test2("\xEF\xBB\xBF" + loon_text, expected);

    // and again with line continuations between every single byte
    test2(insert_line_continuations(loon_text), expected);

    // and again with multiple line continuations and a BOM
    test2("\xEF\xBB\xBF" + insert_line_continuations(loon_text), expected);
}

// feed each of the given 'tests' to the unserialiser, the output
// for each test should equal the given 'expected'
void run_tests(const char * const tests[], const var & expected)
{
    for (const char * const * t = tests; *t; ++t)
        test(*t, expected);
}



void test_simple_valid_loon()
{
    {
        // null
        const char * const tests[] = {
            "null",
            " null ",
            "null;this is a comment",
            "   \t\t   null   \t\t\t\t         ",
            " \t\r\t \n\nnull\n\r\r    \t \t   \t ",
            "null;false",
            0
        };
        run_tests(tests, var::make_null());
    }
    {
        // true
        const char * const tests[] = {
            "true",
            " true ",
            "true;this is a comment",
            "   \t\t   true   \t\t\t\t         ",
            " \t\r\t \n\ntrue\n\r\r    \t \t   \t ",
            "true;false",
            0
        };
        run_tests(tests, var::make_bool(true));
    }
    {
        // false
        const char * const tests[] = {
            "false",
            " false ",
            "false;this is a comment",
            "   \t\t   false   \t\t\t\t         ",
            " \t\r\t \n\nfalse\n\r\r    \t \t   \t ",
            "false;true;false",
            0
        };
        run_tests(tests, var::make_bool(false));
    }
    {
        // empty arry
        const char * const tests[] = {
            "(arry)",
            "(arry )",
            "(arry);this is an empty arry",
            " (arry)",
            " (arry) ",
            " ( arry ) ",
            "(arry)\t",
            "   \t\t   (\tarry\t)   \t\t\t\t         ",
            "\r(       arry)\r",
            "\n(arry         \r)\n",
            "(\narry\n)\r\n",
            "\t(arry\t\t\n\r)",
            " \t\r\t \n\n(arry)\n\r\r    \t \t   \t ",
            "(arry);",
            "(arry);;",
            ";;\n(arry);",
            "(arry) ;",
            "\n;\r (arry);",
            ";;;\t\t;\r(arry);;\n\n;\r",
            "(arry);comment",
            "(;comment\narry;comment\n);comment",
            ";comment\r\n(arry);another\n",
            "(arry);\\",
            "(arry);false",
            0
        };
        run_tests(tests, var::make_arry());
   }
}


void test_valid_loon_strings()
{
    test("\"\"", var(""));
    test("\"hello\"", var("hello"));
    test("\"hello\";comment", var("hello"));

    // UTF-8 
    test("\"G\xC3\xB6""del\"", var("G\xC3\xB6""del"));

    // loon backslash escapes
    test("\"\\\\\"", var("\\"));
    test("\"\\\"\"", var("\""));
    test("\"\\/\"", var("/"));
    test("\"\\b\"", var("\b"));
    test("\"\\f\"", var("\f"));
    test("\"\\n\"", var("\n"));
    test("\"\\r\"", var("\r"));
    test("\"\\t\"", var("\t"));
    test("  \"\\t\";comment  ", var("\t"));
    test("\"\\\\\\\"\\/\\b\\f\\n\\r\\t\"", var("\\\"/\b\f\n\r\t"));
    test("\"\\\\$request:\\\"([^\\\"]+)\\\"\"", var("\\$request:\"([^\"]+)\""));
    test2("\"o\\\r\\\rn\"", var("on")); // {o} {\} {CR} {\} {CR} {n} => {o} {n}

    // UTF-16 \uXXXX and \uXXXX\uYYYY
    test("\"\\u0000\"", var(std::string(1, '\0')));
    test("\"\\u0001\"", var(std::string(1, '\x01')));
    test("\"G\\u00F6del\"", var("G\xC3\xB6""del"));
    {
        const uint8_t s[] = { 0, 1, 2, 0 };
        test("\"\\u0000\\u0001\\u0002\\u0000\"", var(std::string(s, s+sizeof(s))));
    }
    {
        // Unicode Noncharacter (UTF-32 0x0000FFFF, UTF-16 0xFFFF, UTF-8 0xEF 0xBF 0xBF)
        const uint8_t s[] = { 'X', 0xEF, 0xBF, 0xBF, 'Y' };
        test("\"X\\uFFFFY\"", var(std::string(s, s+sizeof(s))));
    }
    {
        // MUSICAL SYMBOL G CLEF (UTF-32 0x0001D11E, UTF-16 0xD834 0xDD1E, UTF-8 0xF0 0x9D 0x84 0x9E)
        const uint8_t s[] = { 0xF0, 0x9D, 0x84, 0x9E };
        test("\"\\uD834\\uDD1e\"", var(std::string(s, s+sizeof(s))));
        const uint8_t t[] = { 0xF0, 0x9D, 0x84, 0x9E, 'A', 'B', 'C' };
        test("\"\\uD834\\uDD1eABC\"", var(std::string(t, t+sizeof(t))));
    }
}


void test_valid_loon_fixnums()
{
    test("0", var(0));
    test("1", var(1));
//TBD    test("-1", var(-1));
    test("123;comment", var(123));
}



void test_valid_loon_numbers()
{
    // make a specialised reader for the sole purpose of testing number parsing
    class number_reader : private loon::reader {
    public:
        number_reader()
        {
            // set the reader up ready to process a stream of loon values
            reader::process_chunk("(arry ", 6, false/*is_last_chunk*/);
        }

        // feed the loon text to be parsed to this function
        void test_number(const std::string & num)
        {
            num_ = num;

            // feed the given number to the reader one character at a time
            const char * p = num.c_str();
            const char * const p_end = p + num.size();
            for (; p != p_end; ++p)
                reader::process_chunk(p, 1, false/*is_last_chunk*/);

            // give the reader some white space, which should cause it
            // to emit the number we just gave it
            got_number_ = false;
            reader::process_chunk(" ", 1, false/*is_last_chunk*/);
            TEST_EQUAL(got_number_, true);
        }

    private:
        std::string num_;
        bool got_number_;

        virtual void loon_dict() {}
        virtual void loon_arry() {}
        virtual void loon_end() {}
        virtual void loon_string(const char *, size_t) {}
        virtual void loon_null() {}
        virtual void loon_bool(bool) {}

        virtual void loon_number(const char * p, size_t len)
        {
            got_number_ = true;
            TEST_EQUAL(std::string(p, p + len), num_);
        }
    };



    const char * const decimal[] = {
        "0",
        "00",
        "000",
        "9",
        "99",
        "999",
        "1234567890",

        "9.",
        "99.",
        "999.",
        "9.9",
        "99.9",
        "999.9",
        "9.99",
        "9.999",
        "999.999",

        "9.e9",
        "99.e9",
        "999.e9",
        "9.e99",
        "99.e99",
        "999.e99",
        "9.9e9",
        "99.9e9",
        "999.9e9",
        "9.99e9",
        "9.999e9",
        "999.999e99",
        "9.e-9",
        "9.9e-9",
        "999.999e-99",
        "9.e+9",
        "9.9e+9",
        "999.999e+99",
        "9e9",
        "99e9",
        "999e9",
        "9e99",
        "99e99",
        "999e99",
        "9e+9",
        "99e+9",
        "999e+9",
        "999e+99",
        "999e-99",

        "9.E9",
        "99.E9",
        "999.E9",
        "9.E99",
        "99.E99",
        "999.E99",
        "9.9E9",
        "99.9E9",
        "999.9E9",
        "9.99E9",
        "9.999E9",
        "999.999E99",
        "9.E-9",
        "9.9E-9",
        "999.999E-99",
        "9.E+9",
        "9.9E+9",
        "999.999E+99",
        "9E9",
        "99E9",
        "999E9",
        "9E99",
        "99E99",
        "999E99",
        "9E+9",
        "99E+9",
        "999E+9",
        "999E+99",
        "999E-99",

        "1e1",
        "0.2",
        "1e00",
        "1.e0",
        "7.8e99",
        "0.9e+9",
        "0.9e-9",
        "0.9E9",
        "9.9E+9",
        "9.9E-9",
        "9.e9",
        "9.e-9",
        "1234567890.0123456789",
        "1234567890.0123456789e0",
        "1234567890.0123456789e01",
        "1234567890.0123456789e012",
        "1234567890.0123456789e+12",
        "1234567890.0123456789e-12",
        "100.001",
        "0.999",

        0
    };
    
    const char * const hex[] = {

        "0x9",
        "0x99",
        "0x999",
        "0xa",
        "0xaa",
        "0xaaa",
        "0xabcdef",
        "0x0123456789abcdef",
        "0x0",
        "0x00",
        "0x000",
        "0x0001",
        "0x00010",
        "0x000100",
        "0xA",
        "0xAA",
        "0xAAA",
        "0xABCDEF",
        "0x0123456789ABCDEF",
        "0X9",
        "0X99",
        "0X999",
        "0XA",
        "0XAA",
        "0XAAA",
        "0XABCDEF",
        "0X0123456789ABCDEF",
        "0xF",
        "0xFF",
        "0xFFF",
        "0xFFFF",
        "0xFFFFF",
        "0xFFFFFF",
        "0xFFFFFFF",
        "0xFFFFFFFF",
        "0xFFFFFFFFF",
        "0xFFFFFFFFFF",
        "0xFFFFFFFFFFF",
        "0xFFFFFFFFFFFF",
        "0xFFFFFFFFFFFFF",
        "0xFFFFFFFFFFFFFF",
        "0xFFFFFFFFFFFFFFF",
        "0xFFFFFFFFFFFFFFFF",

        0
    };

    number_reader reader;

    const std::string plus("+");
    const std::string minus("-");

    for (const char * const * p = decimal; *p; ++p) {
        reader.test_number(*p);
        reader.test_number(plus + *p);
        reader.test_number(minus + *p);
    }

    for (const char * const * p = hex; *p; ++p) {
        reader.test_number(*p);
    }
}






std::string make_random_string()
{
    std::ostringstream oss;
#if 0
    oss
        << "TBD"
        << static_cast<char>(rand() % 0x80)
        << rand();
#endif
    const int len = rand() % 25;
    for (int i = 0; i < len; ++i) {
        if (rand() % 20 == 0)
            oss << static_cast<char>(rand() % 0x20);//TBD UTF-8
        else
            oss << static_cast<char>(rand() % 0x60 + 0x20);//TBD UTF-8
    }

    return oss.str();
}

var make_random_arry(int maxsize);
var make_random_dict(int maxsize);

var make_random_object(int maxsize)
{
    const int size = rand() % maxsize;
    switch (rand() % (maxsize ? 6 : 4)) {
        // 0..3 create leaves only
        case 0: return var::make_null();
        case 1: return var::make_bool(rand() % 2 != 0);
        case 2: return var(rand());
        case 3: return var(make_random_string());
        // 4..5 create branches
        case 4: return make_random_arry(size);
        case 5: return make_random_dict(size);
    }

    return var();
}

var make_random_arry(int maxsize)
{
    const int size = maxsize ? rand() % maxsize : 0;
    var result(var::make_arry(size));
    for (int i = 0; i < size; ++i)
        result[i] = make_random_object(maxsize - 1);
    return result;
}

var make_random_dict(int maxsize)
{
    const int size = maxsize ? rand() % maxsize : 0;
    var result(var::make_dict());
    for (int i = 0; i < size; ++i)
        result[make_random_string()] = make_random_object(maxsize - 1);
    return result;
}

void soaktest()
{
    const int maxsize = 40;
    int iterations = 20;

    while (iterations--) {
        // create a, an arbitrary (random) structure containing arbitrary (random) data
        const var a(make_random_object(maxsize));

        // create b, a loon text representation of a
        const std::string b(serialise(a));
        //std::cout << "\n----------------\n";
        //std::cout << b.size() << '\n';
        //std::cout << b << '\n';

        // create c, a variant unserialised from b
        const var c(unserialise(b));

        // a -> b -> c, if all went well c will contain the same value as the original a
        TEST_EQUAL(c, a);

        // also, test it works with different chunk sizes (sloooooooooooooooow)
        //test(b, a);
    }
}




void test()
{
    test_simple_valid_loon();
    test_valid_loon_strings();
    test_valid_loon_fixnums();
    test_valid_loon_numbers();
    soaktest();
    //test reset() TBD
}

}





std::string escaped(const std::string & s)
{
    return s; //TBD
}

std::string level_indent(int level)
{
    return std::string(level * 4, ' ');
}


std::string serialise(const var & v, bool indent = false, int level = 0)
{
    std::string result(indent ? level_indent(level) : "");

    switch (v.type()) {
    case var::type_null:
        return result + "null\n";

    case var::type_bool:
        return result + (v.as_bool() ? "true" : "false") + "\n";

    case var::type_int:
        {
            std::ostringstream oss;
            oss << v.as_int();
            return result + oss.str() + "\n";
        }

    case var::type_string:
        return result + "\"" + escaped(v.as_string()) + "\"\n";

    case var::type_arry:
        {
            result += "(arry";
            const var::arry_t & a(v.as_arry_t());
            if (!a.empty()) {
                result += "\n";
                for (var::arry_t::const_iterator i = a.begin(); i != a.end(); ++i) {
                    //result += level_indent(level);
                    result += serialise(*i, true, level + 1);
                }
                result += level_indent(level);
            }
            result += ")\n";
            return result;
        }

    case var::type_dict:
        {
            result += "(dict";
            const var::dict_t & a(v.as_dict_t());
            if (!a.empty()) {
                result += "\n";
                for (var::dict_t::const_iterator i = a.begin(); i != a.end(); ++i) {
                    result += level_indent(level + 1) + "\"" + escaped(i->first) + "\" ";
                    result += serialise(i->second, false, level + 1);
                }
                result += level_indent(level);
            }
            result += ")\n";
            return result;
        }
    }

    throw std::exception("serialise unknown type");
}



int main()
{
    try {
        direct_usage_example::test();
        variant_usage_example::test();
        unit_test::test();
    }
    catch (const std::exception & e) {
        std::cout << "std::exception: '" << e.what() << "'\n";
        ++g_fault_count;
    }
    catch (...) {
        std::cout << "exception!\n";
        ++g_fault_count;
    }

    std::cout << "tests executed " << g_test_count;
    std::cout << ", tests failed " << g_fault_count << '\n';
	return g_fault_count ? EXIT_FAILURE : EXIT_SUCCESS;
}
