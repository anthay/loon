# loon-cpp

_Release 1.00_


**Loon** is a data serialisation file format based on S-expressions.
See <http://loonfile.info> for more information about the Loon file format.

**loon-cpp** is a small, public domain C++ library for reading and
writing Loon text. This is the documentation for that library.



## 0. INTRODUCTION

### 0.1 Sample Loon text

Loon is formally documented at <http://loonfile.info>. Here is a brief
introduction to Loon taken from that site:

~~~html
; ### a self-explanatory Loon sample file ###

; - Loon is a data serialisation file format based on S-expressions
; - Loon is designed to be easy for both people and machines to read
; - Loon files are required to be UTF-8 encoded and may have a UTF-8 BOM

; this is a comment - it starts with a semicolon and ends at the line end

; a Loon file contains a single value; in this sample that value is a dict
; structure, which itself may contain many values
(dict
    ; a dict is an unordered list of zero or more key/value pairs enclosed
    ; between "(dict" and ")" tokens;  the key is always a string enclosed
    ; in double quotes and must be unique in the dict
    "key" "value"

    ; the value must be one of these three simple types, or a nested
    ; arry or dict structure or null
    "a number" 1234
    "a boolean" false
    "a string" "any Unicode text except backslash and double quote"
    "a nothing (has no type or value)" null

    ; an arry is an ordered list of zero or more values enclosed between
    ; "(arry" and ")" tokens; the values do not have to be of the same type
    "heterogeneous array" (arry "the" 1 true "brace style")

    "an array of arrays" (arry
        (arry 1 0 0)
        (arry 0 1 0)
        (arry 0 0 1)
    )

    "books" (arry
        (dict
            "name" "Green Eggs and Ham"
            "author" "Dr. Seuss")
        (dict
            "name" "Gödel, Escher, Bach"
            "author" "Douglas Hofstadter")
    )

    "an empty arry" (arry)
    "an empty dict" (dict)
    
    ; white space is needed only where necessary to separate symbols that
    ; would otherwise merge and for human readability
    ; these three key/value pairs have the minimum necessary white space
"one two"(arry 1 2)"twelve"(arry 12)"three"3

    ; bad Loon: examples of ill-formed expressions
    ; (dict "key" 0 "key" 1) - keys not unique
    ; (dict "key")           - missing value
    ; (dict 0 "zero")        - key not a string
    ; (dict "key" hat)       - hat is not a valid Loon value
    ; (dict "key" arry)      - arry doesn't make sense in this context

    ; --- and that's all there is to a Loon file ---

    "Loon" (arry
        "a foolish fellow?"
        "list oriented object notation?"
        "JSON done with S-expressions?"
    )
)
~~~



### 0.2 An overview of the loon-cpp library

- loon-cpp contains a Loon text reader and a Loon text writer. These are
  independent of each other. You can use the reader only, the writer only or
  both together.

- To use the reader you add `src/loon_reader.cpp` to your build and
  `#include src/loon_reader.h` in your source. To use the writer you add
  `src/loon_writer.cpp` to your build and `#include src/loon_writer.h` in your
  source. These four files constitute the loon-cpp library. All other files in
  the distribution are part of the documentation or unit test suite and do not
  need to be included in your build.

- To create a reader you derive a class from `loon::reader::base`. Your class
  must override the `base::loon_XXXX` virtual functions. You feed the Loon text
  to the `base::process_chunk` function. The `loon::reader::base` class will
  invoke the appropriate `loon_XXXX` virtual function for each token it reads
  from the text. Your functions must use these events in a manor appropriate
  to your application.

- `loon::reader::base` is a streaming parser that turns Loon source text into
  a series of tokens and forwards those tokens to higher-level user code as soon
  as they are encountered. This means the reader is small and fast. Also, it can
  process arbitrarily large source text as it does not attempt to parse the
  whole text into an in-memory tree representation. That doesn't stop you
  building a parser on top of `loon::reader::base` that *does* build
  in-memory tree from the Loon tokens, but you are not forced to do this. And
  this is also the disadvantage of the streaming parser: you will need to code
  a parser on top of `loon::reader::base` appropriate to your specific
  application.

- To create a Loon text writer you derive a class from `loon::writer::base`.
  Your class must override the `base::write` virtual function. You invoke the
  `base::loon_XXXX` functions to serialise your data in a manor appropriateto
  your application. `loon::writer::base` class will invoke your write virtual
  function to record the Loon text as it produces it. You can buffer this
  text or write it to a file as required.

- All text produced and consumed by both `loon::reader::base` and
  `loon::writer::base` are assumed to be UTF-8 encoded. loon-cpp components
  do no UTF-8 validation. If you feed UTF-8 text to `loon::reader::base` and
  `loon::writer::base` you will get UTF-8 output. If you feed them invalid
  UTF-8 what you get back is undefined.

- The `loon::reader::base` handles syntax errors in the Loon source text by
  throwing a `loon::reader::exception`. The exception contains some context
  information about the nature and location of the syntax error. If the
  `loon::reader` throws an exception its internal state is undefined. The
  only two safe uses of the reader object are to destroy it or to call
  `loon::reader::base::reset`. The latter action returns the reader to its
  initial state, ready to process Loon text from the beginning.

- The `loon::writer::base` throws no Loon-specific exceptions. (It may throw
  standard library exceptions such as `std::bad_alloc`.) If you do not
  correctly invoke the `loon::writer::base::loon_XXXX` functions to serialise
  your data the Loon text produced will not be well formed or will not
  accurately reflect the data you wish to serialise.




## 1. BUILDING

The loon-cpp library has no separate library build process. Just compile the
reader and/or writer files into your project as described above.



##2. TESTING

The loon-cpp library is distributed with a set of unit tests in the file
`test/test.cpp`. The tutorials are also in this file. Compile this file and run
it like this

### OSX / clang++
~~~bash
$ls
LICENSE		README.md	build		src		test
$cd build/clang/
$ls
makefile
$make test
clang++ -std=c++11 -stdlib=libc++ -Wall -I../../src -c ../../test/test.cpp -o test.o
clang++ -std=c++11 -stdlib=libc++ -Wall -I../../src -c ../../test/var.cpp -o var.o
clang++ -std=c++11 -stdlib=libc++ -Wall -I../../src -c ../../src/loon_reader.cpp -o loon_reader.o
clang++ -std=c++11 -stdlib=libc++ -Wall -I../../src -c ../../src/loon_writer.cpp -o loon_writer.o
clang++ -std=c++11 -stdlib=libc++ -Wall test.o var.o loon_reader.o loon_writer.o -o loontest
./loontest
tests executed 1966, tests failed 0
$
$clang -v
Apple LLVM version 4.2 (clang-425.0.27) (based on LLVM 3.2svn)
Target: x86_64-apple-darwin13.4.0
Thread model: posix
$
~~~

### Windows / MSVC 2010

Open a Visual Studio command prompt (on the Start menu under Microsoft
Visual Studio 2010/Visual Studio Tools). Change to the directory containing
the loon-cpp distribution.
 
~~~html
C:\loon-cpp>dir /b
build
LICENSE
README.md
src
test

C:\loon-cpp>cd build\msvc2010

C:\loon-cpp\build\msvc2010>dir /b
test.vcxproj

C:\loon-cpp\build\msvc2010>msbuild /v:quiet
Microsoft (R) Build Engine version 4.0.30319.18408
[Microsoft .NET Framework, version 4.0.30319.18444]
Copyright (C) Microsoft Corporation. All rights reserved.


C:\loon-cpp\build\msvc2010>dir /b
Debug
test.vcxproj

C:\loon-cpp\build\msvc2010>debug\test
tests executed 1966, tests failed 0

C:\loon-cpp\build\msvc2010>
~~~

You can of course just double-click the `test.vcxproj` file and open the
project in the Visual Studio IDE.



## 3.TUTORIALS



### 3.1 Tutorial 1. Write `std::vector<std::string>` to Loon

#### 3.1.0 Preamble

Given a `std::vector<std::string>`, the Loon text generated by our
serialise() function might look something like this

~~~coffee
(arry
   "I do not like"
   "green eggs"
   "and ham!"
)
~~~


#### 3.1.1 Code

The code for these tutorials is in `test/test.cpp`. Each tutorial is in its own
namespace. Look for `tutorial_1`.

Here is the code for this tutorial. Follow the steps numbered 1 to 6 in the
comments to see how it's done.

~~~cpp
// return the Loon text representing the given vector of UTF-8 strings 'v'
std::string serialise(const std::vector<std::string> & v)
{
    // 1. To use the Loon writer you derive your own writer class from
    // loon::writer::base and provide an implementation for the write()
    // function.
    struct vec_writer : public loon::writer::base {
        std::string str;

        // When the code in loon::writer::base needs to output Loon text
        // it will call this function.
        virtual void write(const char * utf8, int len)
        {
            // You could write the text to a file or whatever you want.
            // Here we just append it to a string.
            str += std::string(utf8, len);
        }
    };

    // 2. Use a vec_writer object to record the elements of the vector.
    vec_writer writer;

    // 3. The obvious way to represent a vector in Loon is with an arry.
    // An arry starts with "(arry", has some arry elements, and ends with ")".
    // To start the arry call loon_arry_begin().
    writer.loon_arry_begin();

    // 4. Next iterate through the vector passing each element to loon_string().
    typedef std::vector<std::string>::const_iterator iter;
    for (iter i = v.begin(); i != v.end(); ++i)
        writer.loon_string(*i);

    // 5. Close the arry with loon_arry_end().
    writer.loon_arry_end();

    // 6. Finally, collect the accumulated Loon text from our vec_writer
    // object and return it to the caller.
    return writer.str;
}
~~~

#### 3.1.2 Test

We can test our code like this

~~~cpp
void test()
{
    // Create a vector and populate it with text.
    std::vector<std::string> v;
    v.push_back("I do not like");
    v.push_back("green eggs");
    v.push_back("and ham!");

    // Create the Loon text from the vector.
    const std::string loon(serialise(v));

    // Check we got what we expected.
    const char * expected = {
        "(arry\n"
        "    \"I do not like\"\n"
        "    \"green eggs\"\n"
        "    \"and ham!\"\n"
        ")"
    };

    // TEST_EQUAL() is a macro that displays an error if its 
    // first argument != second argument
    TEST_EQUAL(loon, expected);
}
~~~



### 3.2 Tutorial 2. Read `std::vector<std::string>` from Loon

#### 3.2.0 Preamble

In this tutorial we'll write an unserialise function that takes text like this

~~~coffee
(arry
   "I do not like"
   "green eggs"
   "and ham!"
)
~~~

and returns it in a `std::vector<std::string>`.


#### 3.2.1 Code

Look for `tutorial_2` in `test/test.cpp`. As before, the numbered steps show how it's done.

~~~cpp
// return a vector of strings initialised from the given UTF-8 Loon text 's'
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
        using base::process_chunk; // just republish the loon::reader::base function

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
            strings.push_back(std::string(utf8, len));
        }

        // 5. We'll get a loon_arry_end event when the ")" token is parsed.
        virtual void loon_arry_end()
        {
            // (In this tutorial we won't do anything with this event.)
        }

        // 6. We won't see any of these events, but we need to provide empty definitions.
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
~~~

#### 3.2.2 Test

~~~cpp
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
~~~



### 3.3 Tutorial 3. Write `std::map<std::string, std::string>` to Loon

#### 3.3.0 Preamble

Similar to Tutorial 1, but this time we'll serialise a map of strings rather
than a vector of strings.

Given a map of strings, the Loon text generated might look something like this

~~~coffee
(dict
   "walk"  "don't run"
   "waving"  "not drowning"
   "all work"  "no play"
)
~~~

#### 3.3.1 Code

~~~cpp
// return the Loon text representing the given map of UTF-8 strings 'm'
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
~~~

#### 3.3.2 Test

~~~cpp
void test()
{
    std::map<std::string, std::string> m;
    m["walk"] = "don't run";
    m["waving"] = "not drowning";
    m["all work"] = "no play";

    const std::string loon(serialise(m));

    // Check we got the text we expected.
    const char * expected = {
        "(dict\n"
        "    \"all work\"  \"no play\"\n"
        "    \"walk\"  \"don't run\"\n"
        "    \"waving\"  \"not drowning\"\n"
        ")"
    };

    TEST_EQUAL(loon, expected);
}
~~~



### 3.4 Tutorial 4. Read `std::map<std::string, std::string>` from Loon


#### 3.4.0 Preamble

Turn Loon text like this

~~~coffee
(dict
   "walk"      "don't run"
   "waving"    "not drowning"
   "all work"  "no play"
)
~~~

into a `std::map<std::string, std::string>`.

In Tutorial 2 we ignored the possibility that the Loon text given to the
unserialiser could be faulty. This time we won't.

There will be two possible outcomes of calling this unserialiser: it will
succeed (and return the map) or it will fail (and throw an exception). If
it fails it will likely be for one of two reasons: the Loon text is not
well formed (not syntactically valid) or the Loon text is well formed but
the data is not in the structure this unserialiser recognises. If it fails
the exception thrown will give the line number of the Loon source text
where the failure was detected and a short message describing the fault.


#### 3.4.1 Code

~~~cpp
// return a map of strings initialised from the given UTF-8 Loon text 's'
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
        using base::process_chunk; // again, just republish the base implementation

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
~~~

#### 3.4.2 Test

~~~cpp
void test()
{
    // Sample Loon.
    const char * loon = {
        "(dict\n"
        "    \"walk\"      \"don't run\"\n"
        "    \"waving\"    \"not drowning\"\n"
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
~~~


### 3.5 Tutorial 5. Read and write all supported Loon data types

#### 3.5.0 Preamble

You're getting the hang of it now. So far we've looked only at
un/serialising strings directly into and out of simple vector and map
structures. This time we'll do something different: we will serialise
and unserialise the full set of Loon data types in any valid structural
arrangement. So the unserialiser will read any valid Loon text into an
in-memory representation (a kind of variant tree) and the serialiser
will take such a representation and generate the corresponding Loon text.

Note that the variant structure (called `var`) used in this tutorial is
not part of the Loon reader or writer. It is just another piece of
scaffolding used to demonstrate Loon API usage. It is also used as part
of the unit testing code where we generate random variants, serialise
them, unserialise the resulting Loon text and finally compare the
unserialised variant with the original.

We'll return to direct C++ structure un/serialisation in Tutorial 6.

#### 3.5.1 Code

Here is the code for the serialise function

~~~cpp
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
~~~

And here is the code for the unserialise function

~~~cpp
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
    using base::process_chunk;

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
~~~

#### 3.5.2 Test

The test code for this tutorial is too long to include in this README. Look
in `test/test.cpp` for `tutorial_5`.


#### 3.5.3 Commentary

In Tutorials 2 and 4 we read Loon text directly into a specific sample C++ data
structure. Those readers were constrained to only interpret Loon text that
corresponded to those data structures. The advantage is that once parsed, you
have the data in the target data structure - you don't have to go querying some
intermediate representation to find the values you need. The disadvantage is
that you'll need to write a specific parser for each data structure you want to
populate from Loon text.

Conversely, this tutorial shows exactly the sort of *read it all into an
in-memory tree representation* parser we've been talking about. It can read any
valid Loon text into a variant data structure (see `test/var.cpp` and
`test/var.h` for the implementation).


### 3.6 Tutorial 6. Model an INI file in Loon

#### 3.6.0 Preamble

In this tutorial we'll model a simple .INI file.

An .INI file might have this structure

~~~ini
[section1]
entry1 = some value 1.1
entry2 = some value 1.2

[section2]
entry1 = some value 2.1
entry2 = some value 2.2
~~~

We could model this in Loon like this

~~~coffee
(dict
    "section1" (dict
        "entry1" "some value 1.1"
        "entry2" "some value 1.2"
    )
    "section2" (dict
        "entry1" "some value 2.1"
        "entry2" "some value 2.2"
    )
)
~~~

We could model it in C++ like this

~~~cpp
// map entry_name -> value
typedef std::map<std::string, std::string> entry_map;

// map section_name -> entry_map (i.e. a collection of entries)
typedef std::map<std::string, entry_map> section_map;
~~~

So we could get and set values like this

~~~cpp
section_map ini;

// set a value
ini["section1"]["entry1"] = "hello world";

// get a value
std::string value = ini["section1"]["entry1"];
~~~

#### 3.6.1 Code

The code below shows how we might un/serialise a section_map from/to Loon.

~~~cpp
// return Loon text representing the given UTF-8 encoded 'ini'
std::string serialise(const section_map & ini)
{
    struct ini_writer : public loon::writer::base {
        std::string str;
        void write(const char * utf8, int len)
        {
            str += std::string(utf8, len);
        }
    };

    ini_writer writer;

    writer.loon_dict_begin();
    {
        typedef section_map::const_iterator sect;
        for (sect s = ini.begin(); s != ini.end(); ++s) {
            writer.loon_dict_key(s->first); // key - section name
            writer.loon_dict_begin(); // value - all entries in this section
            {
                const entry_map & entries(s->second);
                typedef entry_map::const_iterator entr;
                for (entr e = entries.begin(); e != entries.end(); ++e) {
                    writer.loon_dict_key(e->first); // key - entry name
                    writer.loon_string(e->second); // value - entry value
                }
            }
            writer.loon_dict_end();
        }
    }
    writer.loon_dict_end();

    return writer.str;
}


// return a section_map initialised from the given UTF-8 encoded Loon text 's'
section_map unserialise(const std::string & s)
{
    class ini_reader : private loon::reader::base {
    public:
        ini_reader() { reset(); }

        void reset()
        {
            base::reset();
            map_.clear();
            entries_.clear();
            state_ = start;
        }

        using base::process_chunk;

        const section_map & final_value() const
        {
            if (state_ != start && state_ != finish)
                throw std::runtime_error("ini_reader: incomplete");
            return map_;
        }

    private:
        enum {
            start,      // on loon_dict_begin => move to state sections
            sections,   // loon_dict_begin => entries; loon_dict_end => finish
            entries,    // loon_dict_end => sections
            finish      // done, expect no events
        } state_;

        std::string section_name_, entry_name_;
        entry_map entries_;
        section_map map_;

        virtual void loon_dict_begin()
        {
            switch (state_) {
                case start:     state_ = sections;  break;
                case sections:  state_ = entries;   break;
                default:        throw std::runtime_error("ini_reader: bad ini");
            }
        }

        virtual void loon_dict_key(const char * utf8, size_t len)
        {
            switch (state_) {
                case sections:  section_name_ = std::string(utf8, len); break;
                case entries:   entry_name_   = std::string(utf8, len); break;
                default:        throw std::runtime_error("ini_reader: bad ini");
            }
        }

        virtual void loon_string(const char * utf8, size_t len)
        {
            switch (state_) {
                case entries:   entries_[entry_name_] = std::string(utf8, len); break;
                default:        throw std::runtime_error("ini_reader: bad ini");
            }
        }

        virtual void loon_dict_end()
        {
            switch (state_) {
                case sections:
                    state_ = finish;
                    break;
                case entries:
                    map_[section_name_] = entries_;
                    entries_.clear();
                    state_ = sections;
                    break;
                default:
                    throw std::runtime_error("ini_reader: bad ini");
            }
        }

        // not expecting any of these
        virtual void loon_arry_begin() { throw std::runtime_error("ini_reader: unexpected arry"); }
        virtual void loon_arry_end() { throw std::runtime_error("ini_reader: internal error"); }
        virtual void loon_null() { throw std::runtime_error("ini_reader: unexpected null"); }
        virtual void loon_bool(bool) { throw std::runtime_error("ini_reader: unexpected bool"); }
        virtual void loon_number(const char *, size_t, loon::reader::num_type)
        {
            throw std::runtime_error("ini_reader: unexpected number");
        }
    };

    ini_reader reader;
    reader.process_chunk(s.c_str(), s.size(), true/*is last chunk*/);
    return reader.final_value();
}
~~~

#### 3.6.2 Test

~~~cpp
void test()
{
    section_map ini;
    ini["section1"]["entry1"] = "some value 1.1";
    ini["section1"]["entry2"] = "some value 1.2";
    ini["section2"]["entry1"] = "some value 2.1";
    ini["section2"]["entry2"] = "some value 2.2";

    const char * expected_loon = {
"(dict\n\
    \"section1\"  (dict\n\
        \"entry1\"  \"some value 1.1\"\n\
        \"entry2\"  \"some value 1.2\"\n\
    )\n\
    \"section2\"  (dict\n\
        \"entry1\"  \"some value 2.1\"\n\
        \"entry2\"  \"some value 2.2\"\n\
    )\n\
)"
    };

    const std::string loon(serialise(ini));
    TEST_EQUAL(loon, expected_loon);

    const section_map outy(unserialise(loon));
    TEST_EQUAL(outy, ini);
}
~~~

#### 3.6.3 Commentary

In this tutorial we assumed INI section names are unique and
within each section entry names are unique. Some INI files are
not like this and may have several entries in a given section
with the same name. To model such a structure in Loon you'd
probably use arrys instead of dicts.

We again used a state machine to keep track of the parse, which
for complex structures is going to be rather tedious and error-prone.
But the benefit is that the Loon data is read directly
into the target data structure, which is potentially fast and
memory efficient. If you don't want to derive a reader for every
data structure you could just create one reader to read the Loon
data into a variant, as we did in Tutorial 5, which you
would then have to query as necessary.




## 4. REFERENCE

This will have to suffice for the reference...

### 4.0 `loon::writer::base`

From `src/loon_writer.h`

~~~cpp
// The output of this class is written through this function.
// You must override it to collect the Loon text produced.
virtual void write(const char * utf8, int len) = 0;

// Call this function to open a Loon arry.
void loon_arry_begin();
// Call this function to close a Loon arry.
void loon_arry_end();

// Call this function to open a Loon dict.
void loon_dict_begin();
// Call this function to close a Loon dict.
void loon_dict_end();
// Every entry in a Loon dict must have first a key, and then a value.
// Call this function with the key name, which must be UTF-8 encoded.
void loon_dict_key(const std::string & key_name);

// Output the Loon value null.
void loon_null();
// Output the Loon value true or false.
void loon_bool(bool value);
// Output a Loon format decimal integer.
void loon_dec_u32(uint32_t n);
// Output a Loon format decimal integer.
void loon_dec_s32(int32_t n);
// Output a Loon format decimal integer.
void loon_hex_u32(uint32_t n);

// Output a Loon format decimal fraction.
// Note: Loon does not support numeric values that cannot be represented
// as a sequence of digits (such as Infinity and NaN).
void loon_double(double n);

// Output a Loon format string. The given value must be UTF-8 encoded.
// The writer will automatically escape control characters and {\} and {"}.
void loon_string(const std::string & value);

// Call this function to output a pre-formatted Loon value.
// The value is passed unchanged to the write() function.
// Normally you would use one of the above Loon value functions
// rather than this one. If you use this one it is your responsibility
// to ensure that the value is in valid Loon format.
void loon_preformatted_value(const char * utf8, int len);

// Turn "pretty" (indented) printing on or off.
void set_pretty(bool on) { pretty_ = on; }

// Set the number of spaces per indentation level. (Default is 4.)
void set_spaces_per_indent(int n) { spaces_per_indent_ = n; }
~~~

### 4.1 `loon::reader::base`

From `src/loon_reader.h`

~~~cpp
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
~~~



### 4.2 `loon::reader::exception`

If `loon::reader::base` encounters a syntax error it will throw a
`loon::reader::exception`. You can catch this via the exception base class with

~~~cpp
catch (const std::runtime_error & e) {
    std::clog << e.what() << '\n';
}
~~~

or directly with

~~~cpp
catch (const loon::reader::exception & e) {
    std::clog << e.what() << '\n';
}
~~~

If you do the latter you can get access to the exception id and the line number
of the Loon input text where the syntax error was detected. For example

~~~cpp
catch (const loon::reader::exception & e) {
    std::clog << "error " << e.what() << '\n';
    std::clog << "on line " << e.line() << '\n';
    if (e.id() == loon::reader::bad_number) {
        // whatever . . .
    }  
}
~~~

The exception ids and their symbolic names are

~~~cpp
enum error_id {
    no_error                                = 0,
    // no exception will be thrown with this id

    bad_number                              = 100,
    // The parser encountered a number containing invalid characters.
    // (e.g. 99abc, 9e+, are not a valid numbers.)

    bad_hex_number                          = 101,
    // The parser encountered a hexadecimal number containing invalid characters.
    // (e.g. 0x9X is not a valid number.)

    dict_key_is_not_string                  = 102,
    // The parser encountered a dict key that was not a string. A dict is a list of
    // zero or more key/value pairs where the value may be any object but the key must
    // be a string. For example, (dict "key" 123) is valid but (dict true 123) is not.

    incomplete_hex_number                   = 103,
    // A number that started '0x' was not followed by at least one valid hexadecimal digit. (0-9a-fA-F)

    missing_arry_or_dict_symbol             = 104,
    // Something other than 'arry' or 'dict' was found immediately after an open bracket.

    missing_dict_value                      = 105,
    // The dict has a key with no associated value. For example, (dict "key").

    unbalanced_close_bracket                = 106,
    // The parser encountered a close bracket for which there was no corresponding open bracket.

    unclosed_list                           = 107,
    // The input text ended while there is at least one list that has not been closed. E.g. "(arry 1 2 3".

    unclosed_string                         = 108,
    // The input text ended before the closing double quote in the string token.

    unescaped_control_character_in_string   = 109,
    // There is a character between U+0000 and U+001F inclusive or U+007F in the string token.
    // These are not allowed. To include such a character use either the UTF-16 escape (\uXXXX)
    // or other backslash escape sequences such as \n.

    unexpected_or_unknown_symbol            = 110,
    // The parser encountered a symbol it did not expect or did not recognise.
    // For example, "(arry arry)" - the second arry is unexpected.
    // For example, "(arry cake)" - cake is unknown.

    string_escape_incomplete                = 111,
    // The string ended before the backslash escape sequence was completed.

    string_escape_unknown                   = 112,
    // Within a string the backslash escape was not followed by one of these characters:
    // {\} {"} {/} {b} {f} {n} {r} {t} {u}.

    bad_utf16_string_escape                 = 113,
    // Within a string the backslash {u} escape was not followed by four hexadecimal digits (e.g. \u12AB).

    bad_or_missing_utf16_surrogate_trail    = 114,
    // Within a string a backslash {u} UTF-16 escape sequence was encountered that is
    // a UTF-16 surrogate lead value, but this was not followed by a valid UTF-16 surrogate
    // trail value. (A surrogate lead is in the range \uD800...\uDBFF, a surrogate trail is
    // in the range \uDC00...\uDFFF.)

    orphan_utf16_surrogate_trail            = 115,
    // Within a string a backslash {u} UTF-16 escape sequence was encountered that is
    // a UTF-16 surrogate trail value, but this was not preceded by a valid UTF-16 surrogate
    // lead value. (A surrogate lead is in the range \uD800...\uDBFF, a surrogate trail is
    // in the range \uDC00...\uDFFF.)


    // the following should never occur... the code is broken... please report to author...
    internal_error_unknown_state            = 0xBADC0DE1,
    internal_error_inconsistent_state       = 0xBADC0DE2,
};
~~~


## 5. RELEASE NOTES

### loon-cpp release 1.00

#### Known bugs

There are no known bugs in this release. That doesn't mean there
aren't any and if you find one please let the author know.



## 6. LICENSE

**loon-cpp** is hereby released into the public domain by its author

>This is free and unencumbered software released into the public domain.

>Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

>In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

>For more information, please refer to <http://unlicense.org/>



## 7. AUTHOR

**loon-cpp** was made by Anthony Hay in 2014 in Wiltshire, England.  
Contact <mailto:ant@logfilerobot.com>.
