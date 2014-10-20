# loon-cpp

**Loon** is a data serialisation file format based on S-expressions.
See <http://loonfile.info> for more information about the Loon file format.

**loon-cpp** is a small, public domain C++ library for reading and
writing Loon text. This is the documentation for that library.


## 0. INTRODUCTION

### 0.1 Sample Loon text

Loon is formally documented at <http://loonfile.info>. Here is a brief
introduction to Loon taken from that site:

~~~
; ### a self-explanitory Loon sample file ###

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

- loon-cpp contains a Loon text reader and a Loon text writer. These are independent of each other. You can use the reader only, the writer only or both together.

- To use the reader you add `src/loon_reader.cpp` to your build and `#include src/loon_reader.h` in your source. To use the writer you add `src/loon_writer.cpp` to your build and `#include src/loon_writer.h` in your source. These four files constitute the loon-cpp library. All other files in the distribution are part of the loon-cpp documentation or unit test suite and do not need to be included in your build.

- To create a Loon text reader you derive a class from `loon::reader::base`. Your class must override the `base::loon_XXXX` virtual functions. You feed the Loon text to the `base::process_chunk` function. The `loon::reader::base` class will invoke the appropriate `loon_XXXX` virtual function for each Loon token it reads from the text. Your functions must use these events in a manor appropriate to your application.

- To create a Loon text writer you derive a class from `loon::writer::base`. Your class must override the `base::write` virtual function. You invoke the `base::loon_XXXX` functions to serialise your data in a manor appropriate to your application. The `loon::writer::base` class will invoke your write virtual function to record the Loon text as it produces it. You can buffer this text or write it to a file as required.

- All text produced and consumed by both `loon::reader::base` and `loon::writer::base` are assumed to be UTF-8 encoded. loon-cpp components do no UTF-8 validation. If you feed UTF-8 text to `loon::reader::base` and `loon::writer::base` you will get UTF-8 output. If you feed them invalid UTF-8 what you get back is undefined.

- The `loon::reader::base` handles syntax errors in the Loon source text by throwing a `loon::reader::exception`. The exception contains some context information about the nature and location of the syntax error. If the `loon::reader` throws an exception its internal state is undefined. The only two safe uses of the reader object are to destroy it or to call `loon::reader::base::reset`. The latter action returns the reader to its initial state, ready to process Loon text from the beginning.

- The `loon::writer::base` throws no Loon-specific exceptions. (It may throw standard library exceptions such as `std::bad_alloc`.) If you do not correctly invoke the `loon::writer::base::loon_XXXX` functions to serialise your data the Loon text produced will not be well formed or will not accurately reflect the data you wish to serialise.


### 0.2 `loon::reader`

`loon::reader` is a streaming parser. This means it turns loon source text into a series of tokens and forwards those tokens to higher-level user code as soon as they are encountered. The advantages of this are that `loon::reader` is very small and fast. Also, `loon::reader` can process arbitrarily large loon source text as it does not attempt to parse the whole text into an in-memory tree representation. That doesn't stop you building a parser on top of `loon::reader` that *does* build an in-memory tree from the loon tokens, but you are not forced to do this. And this is also the disadvantage of the streaming parser: you will need to code a parser on top of `loon::reader` appropriate to your specific application.

`test/test.cpp` provides two sample parsers: `direct_usage_example` and `variant_usage_example`.

The `direct_usage_example` parser reads loon text directly into a specific sample C++ data structure. It is therefore constrained to only interpret loon text that corresponds to that data structure. The advantage is that once parsed, you have the data in the target data structure - you don't have to go querying some intermediate representation to find the values you need. The disadvantage is that you'll need to write a specific parser for each data structure you want to populate from loon text.

Conversely, the `variant_usage_example` parser is exactly the sort of *read it all into an in-memory tree representation* parser we've been talking about. It can read any valid loon text into a variant data structure called `var` (see `test/var.cpp` and `test/var.h` for the implementation). Just to be clear, `var` is *not* part of `loon::reader`. It is simply a convenient implementation of a variant data type used to both demonstrate possible `loon::reader` usage and to exercise the code as part of the loon-cpp test suite.


### 0.3 `loon::writer`

Readability: by default loon::writer indents each dict key/value pair and each arry value to make the structure of the data more human-readable. This behaviour can be suppressed by setting the no_indent flag.

In indent mode the writer will allign each arry value virtically below the previous value, like this

~~~
; a vertical arrangement may be exactly what you want in some cases...
"loon" (arry
    "a foolish fellow?"
    "list oriented object notation?"
    "JSON done with S-expressions?"
)

; but not in others...
"an array of arrays" (arry
    (arry
        1
        0
        0
    )
    (arry
        0
        1
        0
    )
    (arry
        0
        0
        1
    )
)
~~~

The loon_arry(loon::horiz) paramater switches to a horizontal allignment for the duration of that structure

~~~
; each of the inner arrys was started with the loon::horiz flag...
"an array of arrays" (arry
    (arry 1 0 0)
    (arry 0 1 0)
    (arry 0 0 1)
)
~~~

In no_indent mode no allignment is done.



## 1. BUILDING

bytefluo is a C++ header-only library file; there are no other source
files to compile and no libs to link with. Just include bytefluo.h in
your code and start using it.


##2. TESTING

The bytefluo library is distributed with a self-contained unit test in
the file bytefluo_test.cpp. Compile this file and run it.

### OSX
~~~
 $ g++ -Wall bytefluo_test.cpp
 $ ./a.out
 tests executed N, tests failed 0
~~~

### Windows
~~~
 C:\test> cl /EHsc /W4 /nologo bytefluo_test.cpp
 C:\test> bytefluo_test
 tests executed N, tests failed 0
~~~

(Where N is the number of tests in the current version of the test suite.)

The bytefluo library has been tested by the author under the following
compilers

 - gcc v4.2.1 on OSX
 - Microsoft Visual C++ v7 (2003) on Windows
 - Comeau 4.3.10.1 Beta 2 online in strict C++03 mode


## 3.TUTORIALS

The code for all the following tutorials is in `test/test.cpp`.


### 3.1 Tutorial 1. `std::vector<std::string>` to Loon

Given a `std::vector<std::string>`, the Loon text generated by our
serialise() function might look something like this

~~~
    (arry
        "I do not like"
        "green eggs"
        "and ham!"
    )
~~~

Follow the steps numbered 1 to 6 in the comments to see how it's done.

~~~
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

Then we can test our code like this

~~~
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


### 3.2 Tutorial 2. Loon to `std::vector<std::string>`



## 3. REFERENCE

The unit test source, bytefluo_test.cpp, contains many examples of use.
Here is another:

~~~cpp
 #include "bytefluo.h"

 int main()
 {
     const unsigned char bytes[8] = {
         0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
     };
     bytefluo buf(bytes, bytes + sizeof(bytes), bytefluo::big);
     unsigned long x;
     buf >> x;  // x = 0x00112233
     buf.set_byte_order(bytefluo::little);
     buf >> x;  // x = 0x77665544
 }
~~~

### 3.1  OVERVIEW

The important things to know about bytefluo are

 - When you create a bytefluo object you tell it what data it will
   provide access to, and the initial byte order for any scalar
   reads that will be performed.
 - The bytefluo object does not hold a copy of the given data; it
   merely manages access to that data.
 - The bytefluo object maintains a cursor, which is set initially
   at the start of the managed data. All reads begin at the current
   cursor location and advance the cursor by the size of the data
   read.
 - If a requested read or seek would take the cursor outside the
   bounds of the managed data the read or seek does not take place,
   the state of the bytefluo object remains unchanged and the
   bytefluo object throws a bytefluo_exception object, which is
   derived from std::runtime_error.
 - Assuming that the bytefluo object is managing access to valid
   memory, we provide either the strong guarantee or the nothrow
   guarantee for all operations.
 - The bytefluo implementation is entriely within the header file,
   bytefluo.h. To use the class just #include "bytefluo.h".


### 3.2  DETAIL

#### 3.2.1  CONSTRUCTION

 bytefluo(const void * begin, const void * end, byte_order bo)

The bytefluo object will manage access to the bytes in the half open
range [begin, end). Multi-byte scalars will be read assuming the given
byte order 'bo'. 'bo' must be one of

 bytefluo::big     most-significant byte has lowest address
 bytefluo::little  least-significant byte has lowest address

Throws bytefluo_exception if begin > end or if 'bo' is neither big
nor little.

Example:
 unsigned char foo[99];
 bytefluo buf(foo, foo + sizeof(foo), bytefluo::big);


#### 3.2.2  DEFAULT CONSTRUCTION

 bytefluo()

The bytefluo object will manage access to the empty half open range
[0, 0), with scalar reads defaulting to big-endian. Throws nothing.

Example:
 bytefluo buf;
 size_t siz = buf.size(); // siz = 0
 bool at_end = buf.eos(); // at_end = true


#### 3.2.3  CONSTRUCTION FROM VECTOR

 template <class item_type>
 bytefluo bytefluo_from_vector(
    const std::vector<item_type> & vec,
    bytefluo::byte_order bo)

This free function is provided as a convenient shorthand for
bytefluo(&vec[0], &vec[0] + vec.size(), bo) for a non-empty vector
and bytefluo(0, 0, bo) for an empty vector.

NOTE that any operations on the vector that might change the value
of &vec[0] or vec.size() (e.g. adding elements to the vector may
cause the vector to reallocate its buffer) will silently invalidate
the associated bytefluo object so that attempts to read the vector
contents via that bytefluo object may cause a CRASH.

Example:
 std::vector<unsigned char> vec(99);
 bytefluo buf(bytefluo_from_vector(vec, bytefluo::big));


3.2.4  SET DATA RANGE

 bytefluo & set_data_range(const void * begin, const void * end)

The bytefluo object will manage access to the bytes in the half open
range [begin, end). Throws bytefluo_exception if begin > end.

The cursor is set to the beginning of the range. The current
byte-order setting is unaffected. Returns *this.

Example:
 bytefluo buf;
 size_t siz = buf.size(); // siz = 0
 unsigned char foo[99];
 buf.set_data_range(foo, foo + sizeof(foo));
 siz = buf.size(); // siz = 99


3.2.5  SET BYTE ORDER

 bytefluo & set_byte_order(byte_order bo)

Specify the byte arrangement to be used on subsequent scalar reads.
'bo' must be one of

 bytefluo::big     most-significant byte has lowest address
 bytefluo::little  least-significant byte has lowest address

Throws bytefluo_exception if 'bo' is neither big nor little. The
current data range and cursor position are unaffected. Returns *this.

Example:
 bytefluo buf(...);
 buf.set_byte_order(bytefluo::big);


3.2.6  READ INTEGER SCALAR

 template <typename scalar_type>
 bytefluo & operator>>(scalar_type & out)

Read an integer scalar value from buffer at current cursor position.
The scalar is read assuming the byte order set at construction or
at the last call to set_byte_order(). The cursor is advanced by the
size of the scalar read. Returns *this.

Throws bytefluo_exception if the read would move the cursor after
the end of the managed data range.

Example:
 bytefluo buf(...);
 unsigned short foo, bar;
 buf >> foo >> bar;  // read two successive shorts, foo followed by bar


3.2.7  READ ARBITRARY NUMBER OF BYTES

 bytefluo & read(void * dest, size_t len)

Copy 'len' bytes from buffer at current cursor position to given
'dest' location. The cursor is advanced by the number of bytes
copied. The current byte order setting has no affect on this
operation. Returns *this.

Throws bytefluo_exception if the read would move the cursor after
the end of the managed data range.

Example:
 bytefluo buf(...);
 unsigned char foo[23];
 buf.read(foo, sizeof(foo));


3.2.8  MOVE THE CURSOR

 size_t seek_begin(size_t pos)
 size_t seek_current(long pos)
 size_t seek_end(size_t pos)

These functions move the cursor 'pos' bytes from stream beginning,
the current cursor location and the stream end respectively. They
all return the distance from buffer start to new cursor location.
seek_begin() and seek_end() require pos to be positive; seek_current()
may have a positive (move cursor toward end) or negative (move cursor
toward begin) actual parameter.

Throws bytefluo_exception if the move would put the cursor before
the beginning or after the end of the managed data range.

Example:
 bytefluo buf(...);
 size_t pos = buf.seek_end(3);  // cursor is 3 bytes from buffer end


3.2.9  TEST FOR END OF STREAM

 bool eos() const

Returns true if and only if the cursor is at the end of the stream.
Throws nothing.

Example:
 bytefluo buf(...);
 buf.seek_end(0);
 bool at_end = buf.eos();  // at_end = true


3.2.10  GET STREAM SIZE

 size_t size() const

Returns the number of bytes in the stream. Throws nothing.

Example:
 std::vector<unsigned char> v(99);
 bytefluo buf(bytefluo_from_vector(v, bytefluo::big));
 size_t siz = buf.size(); // siz = 99


3.2.11  GET CURSOR POSITION

 size_t tellg() const

Returns the distance from the buffer start to the current cursor
location. Throws nothing.

Example:
 bytefluo buf(...);
 buf.seek_begin(13);
 buf.seek_current(-3);
 size_t pos = buf.tellg();  // pos = 10


####3.2.12 THE EXCEPTIONS

If something goes wrong the bytefluo object will throw a `bytefluo_exception`
object. You can catch this via the exception base class with

~~~cpp
 catch (const std::runtime_error & e) {
   std::clog << e.what() << '\n';
 }
~~~

or directly with

~~~cpp
 catch (const bytefluo_exception & e) {
   std::clog << e.what() << '\n';
 }
~~~

If you do the latter you can get access to the bytefluo exception id,
for example:

~~~cpp
 catch (const bytefluo_exception & e) {
   std::clog << e.what() << '\n';
   if (e.id() == bytefluo_exception::attempt_to_read_past_end)
     . . .
 }
~~~

The exception ids and their symbolic names are

~~~
 1 null_begin_non_null_end
 2 null_end_non_null_begin
 3 end_precedes_begin
 4 invalid_byte_order
 5 attempt_to_read_past_end
 6 attempt_to_seek_after_end
 7 attempt_to_seek_before_beginning
~~~


## RELEASE NOTES

### loon-cpp v0.01

#### Known Bugs

- The format of decimal fraction output by loon::writer::loon_double() is
dependent on the current locale and therefore will only output decimal
fractions in the required Loon format, with a {.} for a decimal point,
when the locale is set appropriately.
- There are no other known bugs in this release. That doesn't mean there
aren't any so if you find one please let the author know.



## 4. LICENSE

*loon-cpp* is in the public domain. See <http://unlicense.org>
for details of this license.

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



## 5. AUTHOR

*loon-cpp* was made by Anthony Hay in 2014 in Wiltshire, England.  
Contact <mailto:ant@logfilerobot.com>.

 