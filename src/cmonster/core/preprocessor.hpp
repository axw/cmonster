/*
Copyright (c) 2011 Andrew Wilkins <axwalk@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _CMONSTER_CORE_PREPROCESSOR_HPP
#define _CMONSTER_CORE_PREPROCESSOR_HPP

#include <ostream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <clang/Basic/TokenKinds.h>

namespace cmonster {
namespace core {

class FunctionMacro;
class IncludeLocator;
class TokenIterator;
class TokenPredicate;
class Token;

/**
 * The core configurable preprocessor class.
 */
class Preprocessor
{
public:
    virtual ~Preprocessor() {}

    /**
     * Add an include directory.
     *
     * @param path The include directory path to add.
     * @param sysinclude True if path is a system include directory.
     * @return True if the include path was added successfully.
     */
    virtual bool
    add_include_dir(std::string const& path, bool sysinclude = true) = 0;

    /**
     * Define a plain old macro. Equivalent to "#define name value".
     *
     * @param name The name to define.
     * @param value The value to define the macro to (optional).
     * @return True if the macro was defined successfully.
     */
    virtual bool
    define(std::string const& name, std::string const& value="") = 0;

    /**
     * Define a macro that expands by invoking a given callable object.
     *
     * @param name The name of the macro/function that will be replaced in the
     *             output.
     * @param function The function that will be called on expansion.
     * @return True if the macro was defined successfully.
     */
    virtual bool
    define(std::string const& name,
           boost::shared_ptr<FunctionMacro> const& function) = 0;

    /**
     * Adds a pragma handler for the specified string.
     *
     * @param name The name of the pragma to define.
     * @param handler The pragma handler that will be called on interpretation.
     * @return True if the pragma was defined successfully.
     */
    virtual bool
    add_pragma(std::string const& name,
               boost::shared_ptr<FunctionMacro> const& handler) = 0;

    /**
     * Preprocess the input, returning an iterator which will yield the output
     * tokens.
     *
     * The returned iterator must not outlive the preprocessor. The caller is
     * responsible for deleting the object when it is no longer needed.
     *
     * @return A PreprocessorIterator which will yield output tokens, allocated
     *         with "new".
     */
    virtual TokenIterator* create_iterator() = 0;

    /**
     * Tokenize a string.
     *
     * @param s The string to tokenize.
     * @param len The length of the string to tokenize.
     */
    virtual std::vector<cmonster::core::Token>
    tokenize(const char *s, size_t len) = 0;

    /**
     * Create a token from the given "kind" and value.
     *
     * @param kind TODO
     * @param value TODO
     * @param value_len TODO
     */
    virtual Token*
    create_token(clang::tok::TokenKind kind,
                 const char *value = NULL, size_t value_len = 0) = 0;

    /**
     * Preprocess the input and write the result to the specified file
     * descriptor.
     *
     * @param fd TODO
     */
    virtual void preprocess(long fd) = 0;

    /**
     * Lex the next token in the stream.
     */
    virtual Token* next(bool expand = true) = 0;

    /**
     * Format a sequence of tokens.
     */
    virtual std::ostream& format(
        std::ostream &out,
        std::vector<cmonster::core::Token> const& tokens) const = 0;

    /**
     * Set the preprocessor's "include locator", for locating includes
     * externally.
     */
    virtual void
    set_include_locator(boost::shared_ptr<IncludeLocator> const& locator) = 0;
};

}}

#endif

