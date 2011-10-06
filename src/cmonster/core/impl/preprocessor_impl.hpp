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

#ifndef _CMONSTER_CORE_PREPROCESSOR_IMPL_HPP
#define _CMONSTER_CORE_PREPROCESSOR_IMPL_HPP

#include "../preprocessor.hpp"
#include "include_locator_impl.hpp"

#include <clang/Frontend/CompilerInstance.h>

#include <boost/exception_ptr.hpp>

namespace cmonster {
namespace core {
namespace impl {

class TokenSaverPragmaHandler;
class FileChangePPCallback;

class PreprocessorImpl : public Preprocessor
{
public:
    PreprocessorImpl(clang::CompilerInstance &compiler);

    /**
     * @see Preprocessor::add_include_dir.
     */
    bool add_include_dir(std::string const& path, bool sysinclude = true);

    /**
     * @see Preprocessor::define.
     */
    bool define(std::string const& name, std::string const& value="");

    /**
     * @see Preprocessor::define.
     */
    bool define(std::string const& name,
                boost::shared_ptr<FunctionMacro> const& function);

    /**
     * @see Preprocessor::add_pragma.
     */
    bool add_pragma(std::string const& name,
                    boost::shared_ptr<FunctionMacro> const& handler);

    /**
     * @see Preprocessor::create_iterator.
     */
    TokenIterator* create_iterator();

    /**
     * @see Preprocessor::tokenize.
     */
    std::vector<cmonster::core::Token> tokenize(const char *s, size_t len);

    /**
     * @see Preprocessor::create_token.
     */
    Token* create_token(clang::tok::TokenKind kind,
                        const char *value = NULL, size_t value_len = 0);

    /**
     * @see Preprocessor::preprocess.
     */
    void preprocess(long fd);

    /**
     * @see Preprocessor::next.
     */
    Token* next(bool expand = true);

    /**
     * @see Preprocessor::format.
     */
    std::ostream& format(
        std::ostream &out,
        std::vector<cmonster::core::Token> const& tokens) const;

    /**
     * @see Preprocessor::set_include_locator.
     */
    void set_include_locator(boost::shared_ptr<IncludeLocator> const& locator);

    /**
     * Check if an exception is pending, and if so, throw it.
     */
    void check_exception();

    /**
     * @see Preprocessor::getClangPreprocessor.
     */
    const clang::Preprocessor& getClangPreprocessor() const;

private: // Methods
    bool add_pragma(std::string const& name,
                    boost::shared_ptr<FunctionMacro> const& handler,
                    bool with_namespace);

    bool
    add_macro_definition(
        std::string const& name,
        std::vector<cmonster::core::Token> const& value_tokens,
        std::vector<std::string> const& args, bool is_function);

private: // Attributes
    clang::CompilerInstance &m_compiler;
    boost::exception_ptr     m_exception;

    // All of these are owned by the Clang preprocessor object.
    impl::TokenSaverPragmaHandler  *m_token_saver;
    impl::FileChangePPCallback     *m_file_change_callback;
    IncludeLocatorDiagnosticClient *m_include_locator;
};

}}}

#endif

