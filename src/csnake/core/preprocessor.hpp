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

#include <istream>
#include <string>

#include <boost/shared_ptr.hpp>

namespace csnake {
namespace core {

class PreprocessorImpl;
class TokenIterator;
class FunctionMacro;

/**
 * The core configurable preprocessor class.
 */
class Preprocessor
{
public:
    Preprocessor(boost::shared_ptr<std::istream> input,
                 const char *filename = "<input>");

    // void set_language(...)

    /**
     * Add an include path.
     *
     * @param path The include path to add.
     * @param sysinclude True if path is a system include path.
     * @return True if the include path was added successfully.
     */
    bool add_include_path(std::string const& path, bool sysinclude = true);

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
     * Preprocess the input, returning an iterator which will yield the output
     * tokens.
     *
     * The returned iterator must not outlive the preprocessor. The caller is
     * responsible for deleting the object when it is no longer needed.
     *
     * @return A PreprocessorIterator which will yield output tokens, allocated
     *         with "new".
     */
    TokenIterator* preprocess();

private:
    boost::shared_ptr<PreprocessorImpl> m_impl;
};

}}

#endif

