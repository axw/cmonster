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

#ifndef _CSNAKE_CORE_TOKEN_HPP
#define _CSNAKE_CORE_TOKEN_HPP

#include <clang/Lex/Preprocessor.h>

#include <boost/shared_ptr.hpp>

#include <ostream>

namespace cmonster {
namespace core {

class TokenImpl;

class Token
{
public:
    Token();
    Token(clang::Preprocessor &pp);
    Token(clang::Preprocessor &pp, clang::Token const& token);
    Token(clang::Preprocessor &pp,
          clang::tok::TokenKind kind,
          const char *value = NULL, size_t value_len = 0);

    Token(Token const& rhs);
    Token& operator=(Token const& rhs);

    /**
     * Get the underlying Clang token.
     */
    clang::Token& getToken();

    /**
     * Get the underlying Clang token.
     */
    const clang::Token& getToken() const;

    /**
     * Set the underlying Clang token.
     */
    void setToken(clang::Token const&);

    /**
     * Get the token name (stringified "kind").
     */
    const char* getName() const;

private:
    friend std::ostream& operator<<(std::ostream&, Token const& token);

    boost::shared_ptr<TokenImpl> m_impl;
};

/**
 * Output stream operator for Token.
 */
std::ostream& operator<<(std::ostream&, Token const&);

}}

#endif

