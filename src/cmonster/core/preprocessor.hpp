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

#ifndef _CSNAKE_CORE_PREPROCESSOR_HPP
#define _CSNAKE_CORE_PREPROCESSOR_HPP

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <clang/Basic/TokenKinds.h>

namespace cmonster {
namespace core {

class FunctionMacro;
class PreprocessorImpl;
class TokenIterator;
class TokenPredicate;
class Token;

/**
 * The core configurable preprocessor class.
 */
class Preprocessor
{
public:
    Preprocessor(const char *filename,
                 std::vector<std::string> const& include_paths);

    // void set_language(...)

    /**
     * Add an include path.
     *
     * @param path The include path to add.
     * @param sysinclude True if path is a system include path.
     * @return True if the include path was added successfully.
     */
    //bool add_include_path(std::string const& path, bool sysinclude = true);

    /**
     * Define a plain old macro.
     *
     * @param macro The macro string to define.
     * @param predefined True if the macro is a 'predefined' macro, meaning it
     *                   can not be undefined.
     * @return True if the macro was defined successfully.
     */
    bool define(std::string const& macro, bool predefined = false);

    /**
     * Define a macro that expands by invoking a given callable object.
     *
     * @param name The name of the macro/function that will be replaced in the
     *             output.
     * @param function The function that will be called on expansion.
     * @return True if the macro was defined successfully.
     */
    bool define(std::string const& name,
                boost::shared_ptr<FunctionMacro> const& function);

    /**
     * Adds a pragma handler for the specified string.
     *
     * @param name The name of the pragma to define.
     * @param handler The pragma handler that will be called on interpretation.
     * @return True if the pragma was defined successfully.
     */
    bool add_pragma(std::string const& name,
                    boost::shared_ptr<FunctionMacro> const& handler);

    /**
     * Sets a token skipping predicate.
     *
     * @param predicate The predicate to call while iterating, which returns
     *                  true to skip tokens, and, upon returning false, stops
     *                  the token skipping.
     */
    void skip_while(boost::shared_ptr<TokenPredicate> const& predicate);

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
    TokenIterator* create_iterator();

    /**
     * Create a token from the given "kind" and value.
     *
     * @param kind TODO
     * @param value TODO
     * @param value_len TODO
     */
    Token* create_token(clang::tok::TokenKind kind,
                        const char *value = NULL, size_t value_len = 0);

    /**
     * Preprocess the input and write the result to the specified file
     * descriptor.
     *
     * @param fd TODO
     */
    void preprocess(long fd);

    //std::string getSpelling(token_type const& tok) const;

private:
    boost::shared_ptr<PreprocessorImpl> m_impl;
};

}}

#endif

