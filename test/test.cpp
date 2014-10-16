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
#include "loon_writer.h"

#include "var.h" // a sample variant class used for testing, not part of loon itself

#include <iostream>
#include <sstream>


unsigned g_test_count;      // count of number of unit tests executed
unsigned g_fault_count;     // count of number of unit tests that fail

// write a message to std::cout if value == expected_value is false
#define TEST_FAILED()                                   \
{                                                       \
    ++g_fault_count;                                    \
    std::cout                                           \
        << __FILE__ << '(' << __LINE__ << ") : "        \
        << "in '" << __FUNCTION__ << "'"                \
        << " TEST FAILED\n";                            \
}

#define TEST_EQUAL(value, expected_value)               \
{                                                       \
    ++g_test_count;                                     \
    if (!((value) == (expected_value)))                 \
        TEST_FAILED();                                  \
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



#if defined(_MSC_VER) && _MSC_VER < 1700
inline std::string to_string(int n)
{
    return std::to_string(static_cast<long long>(n));
}
#else
inline std::string to_string(int n)
{
    return std::to_string(n);
}
#endif






//////// //     // ////////  ///////  ////////  ////    ///    //          //   
   //    //     //    //    //     // //     //  //    // //   //        ////   
   //    //     //    //    //     // //     //  //   //   //  //          //   
   //    //     //    //    //     // ////////   //  //     // //          //   
   //    //     //    //    //     // //   //    //  ///////// //          //   
   //    //     //    //    //     // //    //   //  //     // //          //   
   //     ///////     //     ///////  //     // //// //     // ////////  //////

namespace tutorial_1 {

/*  This tutorial shows how to serialise a vector of std::strings.
    Given a vector containing three strings, the Loon text generated
    might look something like this

        (arry
            "I do not like"
            "green eggs"
            "and ham!"
        )
*/


// return the Loon text representing the given vector 'v'
std::string serialise(const std::vector<std::string> & v)
{
    // 1. To use the Loon writer you derive your own writer class from
    // loon::writer::base and provide an implementation for the write()
    // function.
    struct vec_writer : public loon::writer::base {
        std::string str;

        // Code in loon::writer::base will call this function when ever
        // it has produced Loon text output.
        virtual void write(const char * utf8, int len)
        {
            // You could write the text to a file or whatever you want.
            // Here we just append it to a string.
            str += std::string(utf8, len);
        }
    };

    // 2. Use your new writer class to record the elements of the vector.
    // Create a vec_writer object
    vec_writer writer;

    // 3. The obvious way to represent a vector in loon is with an arry.
    // An arry starts with "(arry", has some arry elements, and ends with ")".
    // To start the arry call begin_arry()
    writer.loon_arry_begin();

    // 4. In this case each vector element is a string. So we will iterate
    // through the vector passing each element to loon_string().
    // [Note that the strings given to loon_string() must be UTF-8 encoded.]
    typedef std::vector<std::string>::const_iterator iter;
    for (iter i = v.begin(); i != v.end(); ++i)
        writer.loon_string(*i);

    // 5. Close the arry with end_arry()
    writer.loon_arry_end();

    // 6. Finally, collect the accumulated Loon text from our vec_writer
    // object and return it to the caller.
    return writer.str;
}


void test()
{
    // Create a vector and populate it with text.
    std::vector<std::string> v;
    v.push_back("I do not like");
    v.push_back("green eggs");
    v.push_back("and ham!");

    // Use our serialiser to produce Loon text from our vector.
    const std::string loon(serialise(v));

    // Check we got the text we expected.
    const char * expected = {
        "\n" //TBD move this to the end????!!!!
        "(arry\n"
        "    \"I do not like\"\n"
        "    \"green eggs\"\n"
        "    \"and ham!\"\n"
        ")"
    };

    // TEST_EQUAL() is a macro that basically displays an error if
    // the first argument != second argument
    TEST_EQUAL(loon, expected);
}

}



//////// //     // ////////  ///////  ////////  ////    ///    //        ///////  
   //    //     //    //    //     // //     //  //    // //   //       //     // 
   //    //     //    //    //     // //     //  //   //   //  //              // 
   //    //     //    //    //     // ////////   //  //     // //        ///////  
   //    //     //    //    //     // //   //    //  ///////// //       //        
   //    //     //    //    //     // //    //   //  //     // //       //        
   //     ///////     //     ///////  //     // //// //     // //////// ///////// 

namespace tutorial_2 {

// This tutorial demonstrates an unserialise function to produce
// a vector of strings from Loon text.


// return the vector of strings initialised from the given Loon text 's'
std::vector<std::string> unserialise(const std::string & s)
{
    // 1. To use the Loon reader you derive your own reader class
    // from loon::reader::base and provide an implementation
    // for all the loon_XXXX() "event" handlers.
    struct vec_reader : private loon::reader::base  {
        std::vector<std::string> strings;

        // 2. You must pass the raw Loon text to the loon::reader::base
        // parser, which will invoke the appropriate loon_XXXX() event
        // as each token is parsed out of the text. In this application
        // We don't need to do anything more than pass the text straight
        // to the base::process_chunk() function.
        base::process_chunk; // just republish the loon::reader::base function

    private:
        // 3. We'll get a loon_arry_begin event when the "(arry" token is parsed.
        virtual void loon_arry_begin()
        {
            // (In this tutorial we won't do anything with this event.)
        }

        // 4. We'll get one loon_string event for each string in the arry.
        virtual void loon_string(const char * utf8, size_t len)
        {
            // We will append each string to our strings member.
            strings.push_back(std::string(utf8, utf8 + len));
        }

        // 5. We'll get a loon_arry_end event when the ")" token is parsed.
        virtual void loon_arry_end()
        {
            // (In this tutorial we won't do anything with this event.)
        }

        // 6. We won't see any of these events, but we need to provide empty implementations.
        virtual void loon_dict_begin() {}
        virtual void loon_dict_end() {}
        virtual void loon_dict_key(const char *, size_t) {}
        virtual void loon_null() {}
        virtual void loon_bool(bool) {}
        virtual void loon_number(const char *, size_t, loon::reader::num_type) {}
    };

    // 7. Use your new reader class to parse the given Loon text.
    vec_reader reader;

    // 8. Because we were given the complete Loon source text in one string we
    // can feed it all to our parser in one chunk; if we were reading it from
    // a file we could feed it in one arbitrary sized buffer full after
    // another until it was all processed, setting is_last_chunk to true
    // only for the last chunk.
    reader.process_chunk(s.c_str(), s.size(), true/*is last chunk*/);

    // 9. Finally, return the accumulated strings to the caller.
    return reader.strings;
}


void test()
{
    // Here is some sample Loon text.
    const char * loon_text = {
        "(arry \"I do not like\" \"green eggs\" \"and ham!\")"
    };

    // Use our unserialiser to parse the sample text to extract the strings.
    const std::vector<std::string> v(unserialise(loon_text));

    // Check we got the result we expected.
    TEST_EQUAL(v.size(), 3);
    if (v.size() == 3) {
        TEST_EQUAL(v[0], "I do not like");
        TEST_EQUAL(v[1], "green eggs");
        TEST_EQUAL(v[2], "and ham!");
    }
}

}




//////// //     // ////////  ///////  ////////  ////    ///    //        ///////  
   //    //     //    //    //     // //     //  //    // //   //       //     // 
   //    //     //    //    //     // //     //  //   //   //  //              // 
   //    //     //    //    //     // ////////   //  //     // //        ///////  
   //    //     //    //    //     // //   //    //  ///////// //              // 
   //    //     //    //    //     // //    //   //  //     // //       //     // 
   //     ///////     //     ///////  //     // //// //     // ////////  ///////  

namespace tutorial_3 {

/*  This tutorial is similar the Tutorial 1, but this time we'll
    serialise a std::map of std::strings instead of a std::vector
    of std::strings.

    Given a map containing three entries, the Loon text generated
    might look something like this

        (dict
            "all work"  "no play"
            "walk"  "don't run"
            "waving"  "not drowning"
        )
*/


// all strings in given 'm' are assumed to be UTF-8 encoded
std::string serialise(const std::map<std::string, std::string> & m)
{
    // 1. As in Tutorial 1, we derive a writer class from loon::writer::base.
    struct map_writer : public loon::writer::base {
        std::string str;

        virtual void write(const char * utf8, int len)
        {
            str += std::string(utf8, len);
        }
    };

    map_writer writer;

    // 2. A Loon file may contain just one top-level value. Here we use a dict
    // to model the std::map we wish to serialise. First open the dict.
    writer.loon_dict_begin();

    // 3. Write out each key/value pair in the given map.
    typedef std::map<std::string, std::string>::const_iterator iter;
    for (iter i = m.begin(); i != m.end(); ++i) {
        // The dict key must be a string.
        writer.loon_dict_key(i->first);

        // In this case the value is a string too. But in another application
        // it could be any Loon value type, including arry or dict.
        writer.loon_string(i->second);
    }

    // 4. All the key/value pairs have been recorded so we can close the dict.
    writer.loon_dict_end();

    // 5. Finally, return the accumulated Loon text.
    return writer.str;
}


void test()
{
    std::map<std::string, std::string> m;
    m["walk"] = "don't run";
    m["waving"] = "not drowning";
    m["all work"] = "no play";

    const std::string loon(serialise(m));

    // Check we got the text we expected.
    const char * expected = {
        "\n" //TBD move this to the end????!!!!
        "(dict\n"
        "    \"all work\"  \"no play\"\n"
        "    \"walk\"  \"don't run\"\n"
        "    \"waving\"  \"not drowning\"\n"
        ")"
    };

    TEST_EQUAL(loon, expected);
}

}



//////// //     // ////////  ///////  ////////  ////    ///    //       //        
   //    //     //    //    //     // //     //  //    // //   //       //    //  
   //    //     //    //    //     // //     //  //   //   //  //       //    //  
   //    //     //    //    //     // ////////   //  //     // //       //    //  
   //    //     //    //    //     // //   //    //  ///////// //       ///////// 
   //    //     //    //    //     // //    //   //  //     // //             //  
   //     ///////     //     ///////  //     // //// //     // ////////       //  

namespace tutorial_4 {

/*  Turn Loon text like this

        (dict
            "all work"  "no play"
            "walk"      "don't run"
            "waving"    "not drowning"
        )

    into a std::map<std::string, std::string>.

    In Tutorial 2 we ignored the possibility that the Loon text given to the
    unserialiser could be faulty. This time we won't.

    There will be two possible outcomes of calling this unserialiser: it will
    succeed (and return the map) or it will fail (and throw an exception). If
    it fails it will likely be for one of two reasons: the Loon text is not
    well formed (not syntactically valid) or the Loon text is well formed but
    the data is not in the structure this unserialiser recognises. If it fails
    the exception thrown will give the line number of the Loon source text
    where the failure was detected and a short message describing the fault.
*/

// return the map parsed out of the given Loon text 's'
std::map<std::string, std::string> unserialise(const std::string & s)
{
    // 1. As before, we begin by deriving our reader from loon::reader::base.
    class map_reader : private loon::reader::base {

        // 2. We'll use a very simple state machine to keep track of where we are.
        enum {
            start,   // before we get the loon_dict_begin event we'll be in the start state
            in_dict, // after we get loon_dict_begin event we'll be in the in_dict state
            finish   // we'll remain in in_dict until we get the loon_dict_end event
        } state_;

        // We'll accumulate the data here.
        std::map<std::string, std::string> map_;

        // Each dict value will be preceded by a key, which we'll hold temporarily here.
        std::string key_;

    public:
        map_reader()
        {
            reset();
        }

        // 3. Set our reader to it's initial pristine state.
        void reset()
        {
            state_ = start; // start state
            map_.clear();   // with no accumulated data
            base::reset();  // reset the base reader state too
        }

        // 4. The caller must feed the Loon text to be processed to this function.
        base::process_chunk; // again, just republish the base implementation

        // 5. When asked we will return the unserialised map object
        // if, and only if, (iff) the processed text was in the expected
        // format, which we will know because we will have made it through
        // to the finished state.
        std::map<std::string, std::string> final_value() const
        {
            if (state_ != finish) {
                throw std::runtime_error(
                    "map_reader: incomplete source text: missing or unclosed (dict)");
            }
            return map_;
        }


    private:
        // 6. We'll get a loon_dict_begin event when the "(dict" token is parsed.
        virtual void loon_dict_begin()
        {
            if (state_ == start)
                state_ = in_dict; // start -> in_dict
            else {
                throw std::runtime_error(
                    "map_reader: unexpected dict on line " + to_string(current_line()));
            }
        }

        // 7. dict entries come in pairs: here's the key.
        virtual void loon_dict_key(const char * utf8, size_t len)
        {
            // Save the key until we have the associated value.
            key_ = std::string(utf8, len);

            // One thing loon::reader:::base does not do is guarantee that the
            // keys are unique. That's up to us to check for.
            if (map_.find(key_) != map_.end()) {
                throw std::runtime_error(
                    "map_reader: key '" + key_ +
                    "' not unique on line " + to_string(current_line()));
            }
        }

        // 8. And here's the value (always a string in our case).
        virtual void loon_string(const char * utf8, size_t len)
        {
            // Now we have the key/value pair we can record them in our map.
            map_[key_] = std::string(utf8, utf8 + len);
        }

        // 9. We'll get a loon_dict_end event when the ")" token is parsed.
        virtual void loon_dict_end()
        {
            state_ = finish; // in_dict -> finish
        }

        // 10. We shouldn't see any of these events, but if we do it signals we
        // were given Loon text that does not meet the required form for this app.
        virtual void loon_arry_begin()
        {
            throw std::runtime_error("map_reader: unexpected arry on line "
                + to_string(current_line()));
        }
        virtual void loon_arry_end() {}
        virtual void loon_null()
        {
            throw std::runtime_error("map_reader: unexpected null on line "
                + to_string(current_line()));
        }
        virtual void loon_bool(bool)
        {
            throw std::runtime_error("map_reader: unexpected true/false on line "
                + to_string(current_line()));
        }
        virtual void loon_number(const char *, size_t, loon::reader::num_type)
        {
             throw std::runtime_error("map_reader: unexpected number on line "
                + to_string(current_line()));
       }
    };

    // 11. Finally, we use the reader class we just made to parse the given
    // Loon text and return the result to the caller.
    map_reader reader;
    reader.process_chunk(s.c_str(), s.size(), true/*is last chunk*/);
    return reader.final_value();
}


void test()
{
    // Sample Loon.
    const char * loon = {
        "(dict\n"
        "    \"walk\"  \"don't run\"\n"
        "    \"waving\"  \"not drowning\"\n"
        "    \"all work\"  \"no play\"\n"
        ")"
    };

    std::map<std::string, std::string> m;

    try {
        // Load data from sample Loon text.
        m = unserialise(loon);
    }
    catch (const loon::reader::exception & e) {
        std::cout
            << "got loon::reader::exception: " << e.what()
            << "\ndetected on line " << e.line()
            << "\nLoon error id " << e.id() // see loon::reader::error_id for codes
            << "\n";
        TEST_FAILED();
    }
    catch (const std::runtime_error & e) {
         std::cout
            << "got exception " << e.what()
            << "\n";
         TEST_FAILED();
    }

    // Check we got what we expect.
    TEST_EQUAL(m.size(), 3);
    TEST_EQUAL(m["walk"], "don't run");
    TEST_EQUAL(m["waving"], "not drowning");
    TEST_EQUAL(m["all work"], "no play");
}
}




namespace tutorial_5_6 {

//////// //     // ////////  ///////  ////////  ////    ///    //       //////// 
   //    //     //    //    //     // //     //  //    // //   //       //       
   //    //     //    //    //     // //     //  //   //   //  //       //       
   //    //     //    //    //     // ////////   //  //     // //       ///////  
   //    //     //    //    //     // //   //    //  ///////// //             // 
   //    //     //    //    //     // //    //   //  //     // //       //    // 
   //     ///////     //     ///////  //     // //// //     // ////////  //////  


/*  You're getting the hang of it now. So far we've looked only at
    un/serialising strings directly into and out of simple vector and map
    structures. This time we'll do something different: we will serialise
    and unserialise the full set of Loon data types in any valid structural
    arrangement. So the unserialiser will read any valid Loon text into an
    in-memory representation (a kind of variant tree) and the serialiser
    will take such a representation and generate the corresponding Loon text.

    Note that the variant structure used in this tutorial is not part of the
    Loon reader or writer. It is just another piece of scaffolding used to
    demonstrate Loon API usage. It is also used as part of the unit testing
    code where we generate random variants, serialise them, unserialise the
    resulting Loon text and finally compare the unserialised variant with
    the original.

    We'll return to direct C++ structure un/serialisation in Tutorials 7 and 8.
*/


// 1. You know the drill: first derive a writer class from loon::writer::base
// and provide an implementation for the write() function.
struct variant_writer : public loon::writer::base {
    std::string str;

    virtual void write(const char * utf8, int len)
    {
        str += std::string(utf8, len);
    }
};

// 2. As the variant structure is fractal-like where each node (i.e.
// each variant) is either a leaf (true, false, null, string or number)
// or a collection of variants (arry or dict) we'll use a helper function
// that can be called recursively to serialise it:

// output given 'v' to given Loon writer 'writer'
void serialise(const var & v, variant_writer & writer)
{
    switch (v.type()) {
    // 3. Use the appropriate writer function to record the variant
    // leaf value.
    case var::type_null:    writer.loon_null();                 break;
    case var::type_bool:    writer.loon_bool(v.as_bool());      break;
    case var::type_string:  writer.loon_string(v.as_string());  break;
    case var::type_int:     writer.loon_dec_s32(v.as_int());    break;
    case var::type_float:   writer.loon_double(v.as_float());   break;

    case var::type_arry:
        {
            // 4. If 'v' is an array of variants we'll iterate over the
            // values calling serialise() recursively to generate the
            // appropriate Loon text for each value.
            writer.loon_arry_begin();
            const var::arry_t & a(v.as_arry_t());
            if (!a.empty()) {
                for (var::arry_t::const_iterator i = a.begin(); i != a.end(); ++i) {
                    serialise(*i, writer);
                }
            }
            writer.loon_arry_end();
        }
        break;

    case var::type_dict:
        {
            // 5. If 'v' is a dictionary of strings->variants we'll iterate
            // over the pairs outputting first the key then calling serialise()
            // recursively to generate the appropriate Loon text for each
            // associated value.
            writer.loon_dict_begin();
            const var::dict_t & a(v.as_dict_t());
            if (!a.empty()) {
                for (var::dict_t::const_iterator i = a.begin(); i != a.end(); ++i) {
                    writer.loon_dict_key(i->first); // write the key
                    serialise(i->second, writer);   // write the value
                }
            }
            writer.loon_dict_end();
        }
        break;

    default: // (should never happen)
        throw std::runtime_error("unknown variant type");
    }
}

// return a string of Loon text representing the given variant 'v'
std::string serialise(const var & v)
{
    // 6. Now we can use our helper function to serialise the given
    // structure from the root node, the given 'v'
    variant_writer writer;
    serialise(v, writer);
    return writer.str;
}

// I bet that wasn't as hard as you thought it would be. The associated
// test code is below Tutorial 6.




//////// //     // ////////  ///////  ////////  ////    ///    //        ///////  
   //    //     //    //    //     // //     //  //    // //   //       //     // 
   //    //     //    //    //     // //     //  //   //   //  //       //        
   //    //     //    //    //     // ////////   //  //     // //       ////////  
   //    //     //    //    //     // //   //    //  ///////// //       //     // 
   //    //     //    //    //     // //    //   //  //     // //       //     // 
   //     ///////     //     ///////  //     // //// //     // ////////  ///////  

// Unserialise any valid Loon text into a variant.


// 1. As ever, we first derive a class from loon::reader::base and provide
// an implementation for all the loon_XXXX() "events".
class variant_reader : private loon::reader::base {

    // 2. Because of the recursive nature of Loon we'll use a stack to
    // keep track of where we are reading one Loon object so if we have
    // to go off and read an embedded Loon object we'll know where we
    // were when we come back to the first object.
    struct node {
        var value;
        std::string key;
        node()  {}
        node(const var & v) : value(v) {}
    };
    std::vector<node> stack_;

public:
    variant_reader()
    {
        reset();
    }

    // 3. Set our reader to it's initial pristine state.
    void reset()
    {
        base::reset();
        stack_.clear();
    }

    // 4. The caller must feed the Loon text to be processed to this function.
    // (republish base function as there is nothing additional to do here)
    base::process_chunk;

    // 5. When asked we will return the unserialised variant object
    // iff the processed text was in the expected format, which we will
    // know because there will be exactly one variant on the stack_.
    const var & final_value() const
    {
        if (stack_.size() != 1)
            throw std::runtime_error("variant_reader: incomplete");
        return stack_.back().value;
    }

private:
    // 6. Create the appropriate variant object to reflect the given Loon events.

    virtual void loon_arry_begin() { stack_.push_back(node(var::make_arry())); }
    virtual void loon_dict_begin() { stack_.push_back(node(var::make_dict())); }

    virtual void loon_arry_end() { loon_end(); }
    virtual void loon_dict_end() { loon_end(); }

    void loon_end()
    {
        switch (stack_.size()) {
        case 0:
            // loon::reader::base will never give a loon_arry_end or loon_dict_end
            // event with no prior loon_dict_begin or loon_arry_begin event
            throw std::runtime_error("variant_reader: internal error"); // (should never happen)

        case 1:
            // this closes the outer dict or arry; parsing should now be complete
            // and the value of the parse is the one and only variant on the stack
            break;

        default:
            // this closes an inner dict or arry
            {
                // get the newly completed node n
                const node n(stack_.back()); stack_.pop_back();

                // insert n.value into the arry or dict now at the top of the stack
                node & top = stack_.back();
                if (top.value.type() == var::type_arry)
                    top.value.push_back(n.value);
                else if (top.value.type() == var::type_dict) {
                    top.value[top.key] = n.value;
                }
                else
                    throw std::runtime_error("variant_reader: internal error");
            }
            break;
        }
    }

    // [utf8, utf8+len) is a UTF-8 encoded string that is a Loon dict key
    virtual void loon_dict_key(const char * utf8, size_t len)
    {
        // One thing loon::reader:::base does not do is guarantee that the
        // keys are unique. That's up to us to check for.
        const std::string key(utf8, len);
        if (stack_.back().value.key_exists(key)) {
            throw std::runtime_error(
                "variant_reader: key '" + key +
                "' not unique on line " + to_string(current_line()));
        }
        stack_.back().key = key;
    }

    // [utf8, utf8+len) is a UTF-8 encoded Loon string value
    virtual void loon_string(const char * utf8, size_t len)
    {
        const std::string value(utf8, len);
        if (stack_.empty())
            stack_.push_back(node(var(value)));
        else {
            node & top = stack_.back();
            if (top.value.type() == var::type_arry)
                top.value.push_back(var(value)); // given string is arry value
            else if (top.value.type() == var::type_dict) {
                top.value[top.key] = var(value); // given string is the dict value
            }
            else {
                // loon::reader should never allow this
                throw std::runtime_error("variant_reader: internal error");
            }
        }
    }

    // [p, p+len) is a string representing either a hex or decimal integer
    // or a decimal floating point number, as indicated by 'ntype'
    virtual void loon_number(const char * p, size_t len, loon::reader::num_type ntype)
    {
        switch (ntype) {
        case loon::reader::num_dec_int:
        case loon::reader::num_hex_int:
            new_leaf(var(std::stoi(std::string(p, len), 0, 0)));
            break;

        case loon::reader::num_float:
            new_leaf(var(std::stod(std::string(p, len), 0)));
            break;
        }
    }

    // true, false and null Loon symbols
    virtual void loon_bool(bool value) { new_leaf(var::make_bool(value)); }
    virtual void loon_null()           { new_leaf(var::make_null()); }

    void new_leaf(const var & value)
    {
        if (stack_.empty())
            stack_.push_back(node(value));
        else {
            node & top = stack_.back();
            if (top.value.type() == var::type_arry)
                top.value.push_back(value);
            else if (top.value.type() == var::type_dict) {
                top.value[top.key] = value;
            }
            else
                throw std::runtime_error("variant_reader: internal error");
        }
    }
};


// create a variant object from the given Loon text 't'
var unserialise(const std::string & t)
{
    // 7. Finally, we use the reader class we just made to parse the given
    // Loon text and return the result to the caller.
    variant_reader reader;
    reader.process_chunk(t.c_str(), t.size(), true/*is last chunk*/);
    return reader.final_value();
}



void test()
{
    // Use some arbitrary Loon sample text to test our variant code.
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
    
    // "Manually" create a variant object containing the same data as the Loon sample text.
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


    // First try unserialising the sample text to see if we get the expected result.
    const var a(unserialise(loon_text));
    TEST_EQUAL(a, expected);


    const char * expected_loon = {
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
    };

    // Now try serialising the variant to see if we get the expected serialisation text.
    const std::string b(serialise(a));
    TEST_EQUAL(b, expected_loon);
}

}



namespace tutorial_7 {

//////// //     // ////////  ///////  ////////  ////    ///    //       //////// 
   //    //     //    //    //     // //     //  //    // //   //       //    // 
   //    //     //    //    //     // //     //  //   //   //  //           //   
   //    //     //    //    //     // ////////   //  //     // //          //    
   //    //     //    //    //     // //   //    //  ///////// //         //     
   //    //     //    //    //     // //    //   //  //     // //         //     
   //     ///////     //     ///////  //     // //// //     // ////////   //     

// No more variants. For the final tutorials we're back to reading Loon
// directly into a C++ data structure.


// Say we have this structure we wish to persist via loon.
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
    void clear() { name.clear(); books.clear(); }
};

/*  When serialised it might look like this:

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


bool operator==(const library::book & lhs, const library::book & rhs)
{
    return lhs.name == rhs.name && lhs.author == rhs.author;
}

bool operator==(const library & lhs, const library & rhs)
{
    return lhs.name == rhs.name && lhs.books == rhs.books;
}


// return the Loon text representing the given structure 'lib'
std::string serialise(const library & lib)
{
    struct lib_writer : public loon::writer::base {
        std::string str;

        virtual void write(const char * utf8, int len)
        {
            str += std::string(utf8, len);
        }
    };

    lib_writer writer;

    // the obvious way to represent a struct in Loon is with a dict
    writer.loon_dict_begin();
    {
        // (it's not necessary to use {blocks}, but they help keep track
        // of the arry and dict structures)

        // the library object has a name field with a string value
        writer.loon_dict_key("name");
        writer.loon_string(lib.name);

        // the library object has vector of books; the most obvious way
        // to serialise that will be as an arry
        writer.loon_dict_key("books");
        writer.loon_arry_begin();
        {
            // serialise each book in the vector
            typedef std::vector<library::book>::const_iterator iter;
            for (iter i = lib.books.begin(); i != lib.books.end(); ++i) {
                // each book is probably best represented as a dict
                writer.loon_dict_begin();
                {
                    writer.loon_dict_key("author");
                    writer.loon_string(i->author);

                    writer.loon_dict_key("name");
                    writer.loon_string(i->name);
                }
                writer.loon_dict_end();
            }
        }
        // close the arry of books
        writer.loon_arry_end();
    }
    // finally, close the top level dict
    writer.loon_dict_end();

    return writer.str;
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
    };

    const std::string serialised_a(serialise(a));
    TEST_EQUAL(serialised_a, expected);
}

}



namespace tutorial_8 {

//////// //     // ////////  ///////  ////////  ////    ///    //        ///////  
   //    //     //    //    //     // //     //  //    // //   //       //     // 
   //    //     //    //    //     // //     //  //   //   //  //       //     // 
   //    //     //    //    //     // ////////   //  //     // //        ///////  
   //    //     //    //    //     // //   //    //  ///////// //       //     // 
   //    //     //    //    //     // //    //   //  //     // //       //     // 
   //     ///////     //     ///////  //     // //// //     // ////////  ///////  


// Read the C++ library structure we showed in Tutorial 7 from Loon text.
using tutorial_7::library;


// return a library object from the given loon text
library unserialise(const std::string & loon_text_utf8)
{
    class lib_reader : private loon::reader::base  {
        //  We'll use a simple state machine to help keep track of things.
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
        /*  OK, it's not as simple as I'd like, in fact it's rather
            tedious and error-prone, but at least the end result will be the
            Loon data read directly into our target data structure, which is
            potentially fast and memory efficient. If you don't want to
            derive a reader for every data structure you could just create
            one reader to read the Loon data into a variant, as we did in
            Tutorials 5 & 6, which you would then have to query as necessary. */

        library::book b_;   // build each book here before adding it to lib_.books
        library lib_;       // accumulate the data here

    public:
        lib_reader()
        {
            reset();
        }

        // reset reader to start state
        void reset()
        {
            base::reset();
            state_ = start;
            b_.clear();
            lib_.clear();
        }

        // feed the loon text to be parsed to this function
        base::process_chunk;

        // return the final result of the parse
        const library & final_value() const
        {
            if (state_ != finish)
                throw std::runtime_error("lib_reader: incomplete");
            return lib_;
        }

    private:
        virtual void loon_dict_begin()
        {
            if (state_ == start)
                state_ = lib;
            else if (state_ == in_books) {
                b_.clear();
                state_ = book;
            }
            else
                throw std::runtime_error("lib_reader: unexpected 'begin_dict'");
        }

        virtual void loon_arry_begin()
        {
            if (state_ == books)
                state_ = in_books;
            else
                throw std::runtime_error("lib_reader: unexpected 'begin_arry'");
        }

        virtual void loon_dict_end()
        {
            if (state_ == lib)
                state_ = finish;
            else if (state_ == book) {
                lib_.books.push_back(b_);
                state_ = in_books;
            }
            else
                throw std::runtime_error("lib_reader: unexpected ')'");
        }

        virtual void loon_arry_end()
        {
            if (state_ == in_books)
                state_ = lib;
            else
                throw std::runtime_error("lib_reader: unexpected ')'");
        }

        virtual void loon_dict_key(const char * utf8, size_t len)
        {
            const std::string value(utf8, len);
            switch (state_) {
            case lib:
                if (value == "name")
                    state_ = lib_name;
                else if (value == "books")
                    state_ = books;
                else
                    throw std::runtime_error("lib_reader: unexpected library dict key '" + value + "'");
                break;
            case book:
                if (value == "name")
                    state_ = book_name;
                else if (value == "author")
                    state_ = book_auth;
                else
                    throw std::runtime_error("lib_reader: unexpected book dict key '" + value + "'");
                break;
            default:
                throw std::runtime_error("lib_reader: unexpected dict key");
            }
        }

        virtual void loon_string(const char * utf8, size_t len)
        {
            const std::string value(utf8, len);

            switch (state_) {
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
                throw std::runtime_error("lib_reader: unexpected string");
            }
        }

        // for our library object the occurance of any of these events would be wrong
        virtual void loon_null() { throw std::runtime_error("lib_reader: unexpected null"); }
        virtual void loon_bool(bool) { throw std::runtime_error("lib_reader: unexpected bool"); }
        virtual void loon_number(const char *, size_t, loon::reader::num_type)
        {
            throw std::runtime_error("lib_reader: unexpected number");
        }
    };


    lib_reader reader;
    reader.process_chunk(loon_text_utf8.c_str(), loon_text_utf8.size(), true/*is last chunk*/);
    return reader.final_value();
}


void test()
{
    const char * loon_sample = {
        "(dict\n"
        "    \"name\"  \"The British Library\"\n"
        "    \"books\"  (arry\n"
        "        (dict\n"
        "            \"author\"  \"Dr. Seuss\"\n"
        "            \"name\"  \"Green Eggs and Ham\"\n"
        "        )\n"
        "        (dict\n"
        "            \"author\"  \"Douglas Hofstadter\"\n"
        "            \"name\"  \"G\\u00F6del, Escher, Bach\"\n"
        "        )\n"
        "    )\n"
        ")"
    };

    const library lib(unserialise(loon_sample));
    TEST_EQUAL(lib.name, "The British Library");
    TEST_EQUAL(lib.books.size(), 2);
    if (lib.books.size()) {
        TEST_EQUAL(lib.books[0].name, "Green Eggs and Ham");
        TEST_EQUAL(lib.books[0].author, "Dr. Seuss");
        TEST_EQUAL(lib.books[1].name, "G\xC3\xB6""del, Escher, Bach");
        TEST_EQUAL(lib.books[1].author, "Douglas Hofstadter");
    }
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
using tutorial_5_6::variant_reader;
using tutorial_5_6::serialise;

// return given 's' with {\} {LF}, {\} {CR} and {\} {CR} {LF} line splice
// sequences inserted before each and every byte
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
    return reader.final_value();
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
    return reader.final_value();
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


void test_strings()
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
    test("\"/\"", var("/")); // NOTE: Loon does not *require* that {/} is escaped
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



void test_numbers()
{
    test("0",           var(0));
    test("1",           var(1));
    test("123",         var(123));
    test("-1",          var(-1));
    test("-123",        var(-123));
    test("123;comment", var(123));

    test("0x0",         var(0));
    test("0x1",         var(1));
    test("0xFFFF",      var(0xFFFF));
    test("0XABCDEF",    var(0xABCDEF));
    test("0XABCDEF;12", var(0xABCDEF));

    test("0.",          var(0.0));
    test("0.1",         var(0.1));
    test("0.1e0",       var(0.1));
    test("1e-1",        var(0.1));
    test("1e+1",        var(10.0));
    test("-99.9",       var(-99.9));
    test("0.;",         var(0.0));


    // make a specialised reader for the sole purpose of testing number parsing
    class number_reader : private loon::reader::base {
    public:
        number_reader()
        {
            // set the reader up ready to process a stream of loon values
            base::process_chunk("(arry ", 6, false/*is_last_chunk*/);
        }

        void test_number(const std::string & num)
        {
            num_ = "#TEST#";

            // feed the given number to the reader one character at a time
            const char * p = num.c_str();
            const char * const p_end = p + num.size();
            for (; p != p_end; ++p)
                base::process_chunk(p, 1, false/*is_last_chunk*/);

            // give the reader some white space, which should cause it
            // to emit the number we just gave it
            TEST_EQUAL(num_, "#TEST#");
            base::process_chunk(" ", 1, false/*is_last_chunk*/);
            TEST_EQUAL(num_, num);
        }

    private:
        std::string num_;

        virtual void loon_dict_begin() {}
        virtual void loon_dict_end() {}
        virtual void loon_arry_begin() {}
        virtual void loon_arry_end() {}
        virtual void loon_dict_key(const char *, size_t) {}
        virtual void loon_string(const char *, size_t) {}
        virtual void loon_null() {}
        virtual void loon_bool(bool) {}

        virtual void loon_number(const char * p, size_t len, loon::reader::num_type)
        {
            num_ = std::string(p, p + len);
        }
    };

    const char * const valid_decimal[] = {
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
    
    const char * const valid_hex[] = {
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

    const std::string plus("+");
    const std::string minus("-");

    number_reader reader;
    for (const char * const * p = valid_decimal; *p; ++p) {
        reader.test_number(*p);
        reader.test_number(plus + *p);
        reader.test_number(minus + *p);
    }

    // hex numbers may not be preceeded with + or -
    for (const char * const * p = valid_hex; *p; ++p)
        reader.test_number(*p);


    const char * const nearly_numbers[] = {
        "9.9e=",
        "9.9e",
        "9.9e+",
        "9.9e-",
        ".9",
        "1.2.3",
        "1.-2",
        "9.9e9e",
        "9.X",
        "0x",
        "1xABCD",
        "0xABCDEFG",
        "9A",
        "9+",
        "+",
        "-",

        0
    };
    for (const char * const * p = nearly_numbers; *p; ++p) {
        number_reader r;
        TEST_EXCEPTION(r.test_number(*p), loon::reader::exception);
    }
}




void expect_exception(
    const std::string & loon,
    loon::reader::error_id id,
    int line)
{
    bool got_exception = false;
    try {
        unserialise(loon);
    }
    catch (const loon::reader::exception & e) {
        got_exception = true;

        TEST_EQUAL(e.id(), id);
        if (e.id() != id) {
            std::cout
                << "[expected exception id " << id
                << " got " << e.id()
                << "]\n"
                << e.what() << "\n";
        }

        TEST_EQUAL(e.line(), line);
        if (e.line() != line) {
            std::cout
                << "[expected exception line number " << line
                << " got " << e.line()
                << "]\n";
        }
        //std::cout << e.what() << "\n";
    }
    TEST_EQUAL(got_exception, true);
    if (!got_exception)
        std::cout << "[expected exception but didn't get one]\n";
}


void test_syntax_errors()
{
    struct test_tuple {
        const char * text;
        int expected_line;
        loon::reader::error_id expected_exception_id;
    };

    using namespace loon::reader;

    const test_tuple bad_loon[] = {
        {"1x",                          1,  bad_number},
        {"1.x",                         1,  bad_number},
        {"9ed",                         1,  bad_number},
        {"9e+",                         1,  bad_number},
        {"9e+x",                        1,  bad_number},
        {"9e9e",                        1,  bad_number},
        {"0x",                          1,  incomplete_hex_number},
        {"0x ",                         1,  incomplete_hex_number},
        {"0xAX",                        1,  bad_hex_number},
        {"()",                          1,  missing_arry_or_dict_symbol},
        {"(999)",                       1,  missing_arry_or_dict_symbol},
        {"(\"abc\")",                   1,  missing_arry_or_dict_symbol},
        {"((arry))",                    1,  missing_arry_or_dict_symbol},
        {"\n(cake)",                    2,  missing_arry_or_dict_symbol},
        {"(\n\ncake)",                  3,  missing_arry_or_dict_symbol},
        {")",                           1,  unbalanced_close_bracket},
        {"(arry\n)\n)\n)",              3,  unbalanced_close_bracket},
        {"(dict",                       1,  unclosed_list},
        {"(arry",                       1,  unclosed_list},
        {"(arry (arry)",                1,  unclosed_list},
        {"(arry 1 2 3",                 1,  unclosed_list},
        {"\"a\nb\"",                    2,  unescaped_control_character_in_string},
        {"\"\1\"",                      1,  unescaped_control_character_in_string},
        {"\"\x1F\"",                    1,  unescaped_control_character_in_string},
        {"\"\\x00\"",                   1,  string_escape_unknown},
        {"\"\\u\"",                     1,  bad_utf16_string_escape},
        {"\"\\u0\"",                    1,  bad_utf16_string_escape},
        {"\"\\u00\"",                   1,  bad_utf16_string_escape},
        {"\"\\u000\"",                  1,  bad_utf16_string_escape},
        {"\"\\u000G\"",                 1,  bad_utf16_string_escape},
        {"\"\\uD834\"",                 1,  bad_or_missing_utf16_surrogate_trail},
        {"\"\\uDD1e\"",                 1,  orphan_utf16_surrogate_trail},
        {"\"a\\\nb",                    2,  unclosed_string},
        {"\"abc",                       1,  unclosed_string},
        {"(dict \"key\")",              1,  missing_dict_value},
        {"(dict 0 \"zero\")",           1,  dict_key_is_not_string},
        {"(dict true \"zero\")",        1,  dict_key_is_not_string},
        {"(dict false \"zero\")",       1,  dict_key_is_not_string},
        {"(dict null \"zero\")",        1,  dict_key_is_not_string},
        {"(dict \"key\" hat)",          1,  unexpected_or_unknown_symbol},
        {"(dict \"key\" arry)",         1,  unexpected_or_unknown_symbol},
        {"(arry arry)",                 1,  unexpected_or_unknown_symbol},
        {"(arry cake)",                 1,  unexpected_or_unknown_symbol},
        {"xyz",                         1,  unexpected_or_unknown_symbol},

        // UTF-8 BOM is {0xEF} {0xBB} {0xBF}; test sequences that almost
        // match the BOM are passed through and come out as unknown symbols
        {"\xEF\xBF\xBD",                1,  unexpected_or_unknown_symbol}, // U+FFFD
        {"\xEF\xBB\xA7",                1,  unexpected_or_unknown_symbol}, // U+FEE7

        {0}
    };

    for (const test_tuple * p = bad_loon; p->text; ++p)
        expect_exception(p->text, p->expected_exception_id, p->expected_line);

    const char s1[] = {'"', '\0', '"' };
    expect_exception(std::string(s1, s1 + sizeof(s1)), unescaped_control_character_in_string, 1);

    // test the reader detects duplicate keys
    {
        bool got_exception = false;
        try {
            unserialise("(dict \"key\" 0 \"key\" 1)");
        }
        catch (const std::runtime_error &) {
            got_exception = true;
            //std::cout << e.what() << "\n";
        }
        TEST_EQUAL(got_exception, true);
    }
}



void test_reset()
{
    variant_reader reader;

    // feed reader some bad text that will throw an exception
    bool got_exception = false;
    try {
        reader.process_chunk("(arry 0x ", 9, false/*is last chunk*/);
    }
    catch (const loon::reader::exception &) {
        got_exception = true;
    }
    TEST_EQUAL(got_exception, true);

    // now feed it some valid text and expect an exception
    got_exception = false;
    try {
        reader.process_chunk("0x9", 3, true/*is last chunk*/);
    }
    catch (const loon::reader::exception &) {
        got_exception = true;
    }
    TEST_EQUAL(got_exception, true);

    // now reset the reader and feed it the same valid text and this time
    // we do not expect an exception
    reader.reset();
    got_exception = false;
    try {
        reader.process_chunk("0x9", 3, true/*is last chunk*/);
    }
    catch (const loon::reader::exception &) {
        got_exception = true;
    }
    TEST_EQUAL(got_exception, false);
    TEST_EQUAL(reader.final_value(), var(9));
}





std::string make_random_string()
{
    std::ostringstream oss;
    const int len = rand() % 25;
    for (int i = 0; i < len; ++i) {
        if (rand() % 20 == 0)
            oss << static_cast<char>(rand() % 0x20);//TBD UTF-8
        else
            oss << static_cast<char>(rand() % 0x60 + 0x20);//TBD UTF-8
    }

    return oss.str();
}

var make_random_object(int);

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

// flip a coin, return true if it comes down heads
bool heads()
{
    return rand() % 2 != 0;
}

var make_random_object(int maxsize)
{
    const int size = rand() % maxsize;
    switch (rand() % (maxsize ? 7 : 5)) {
        // 0..4 create leaves only
        case 0: return var::make_null();
        case 1: return var::make_bool(heads());
        case 2: return var(rand() * (heads() ? 1 : -1));
        case 3: return var((heads() ? 1e20 : 1e-20) * rand() * (heads() ? 1 : -1));
        case 4: return var(make_random_string());
        // 5..6 create branches
        case 5: return make_random_arry(size);
        case 6: return make_random_dict(size);
    }

    return var();
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
    test_strings();
    test_numbers();
    test_syntax_errors();
    test_reset();
    soaktest();
}

}



int main()
{
    try {
        tutorial_1::test();
        tutorial_2::test();
        tutorial_3::test();
        tutorial_4::test();
        tutorial_5_6::test();
        tutorial_7::test();
        tutorial_8::test();
        unit_test::test();
    }
    catch (const std::exception & e) {
        std::cout << "std::exception: " << e.what() << "\n";
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


// comments
// get rid of all TBD
// write README.md
// test on OSX
// publish
