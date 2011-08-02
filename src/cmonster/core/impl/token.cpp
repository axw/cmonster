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

#include "../token.hpp"
#include <stdexcept>

namespace cmonster {
namespace core {

class TokenImpl
{
public:
    TokenImpl(clang::Preprocessor &pp) : preprocessor(pp), token()
    {
        token.startToken();
    }
    TokenImpl(clang::Preprocessor &pp, clang::Token const& token_)
      : preprocessor(pp), token(token_) {}
    clang::Preprocessor &preprocessor;
    clang::Token         token;
};

Token::Token() : m_impl() {}

Token::Token(clang::Preprocessor &pp) : m_impl(new TokenImpl(pp)) {}

Token::Token(clang::Preprocessor &pp, clang::Token const& token)
  : m_impl(new TokenImpl(pp, token)) {}

Token::Token(clang::Preprocessor &pp, clang::tok::TokenKind kind,
             const char *value, size_t value_len)
  : m_impl(new TokenImpl(pp))
{
    clang::Token &token = m_impl->token;
    token.setKind(kind);
    if (token.isAnyIdentifier())
    {
        if (!value || !value_len)
        {
            throw std::invalid_argument(
                "Expected a non-empty value for identifier");
        }
        llvm::StringRef s(value, value_len);
        token.setIdentifierInfo(
            m_impl->preprocessor.getIdentifierInfo(s));
        m_impl->preprocessor.CreateString(value, value_len, token);
    }
    else
    {
        if (!value || !value_len)
        {
            if (token.isLiteral())
            {
                throw std::invalid_argument(
                    "Expected a non-empty value for literal");
            }
            else
            {
                value = clang::tok::getTokenSimpleSpelling(kind);
                if (value)
                    value_len = strlen(value);
            }
        }
        // Must use this, as it stores the value in a "scratch buffer" for
        // later reference.
        pp.CreateString(value, value_len, token);
    }
}

Token::Token(Token const& rhs) : m_impl(new TokenImpl(*rhs.m_impl))
{
}

Token& Token::operator=(Token const& rhs)
{
    // XXX can we do some kind of COW here?
    if (rhs.m_impl)
        m_impl.reset(new TokenImpl(*rhs.m_impl));
    else
        m_impl.reset();
    return *this;
}

void Token::setToken(clang::Token const& token)
{
    if (!m_impl) // XXX make "undefined behaviour"?
        throw std::runtime_error("Invalid use of Token");
    m_impl->token = token;
}

clang::Token& Token::getToken()
{
    if (!m_impl) // XXX make "undefined behaviour"?
        throw std::runtime_error("Invalid use of Token");
    return m_impl->token;
}

const clang::Token& Token::getToken() const
{
    if (!m_impl) // XXX make "undefined behaviour"?
        throw std::runtime_error("Invalid use of Token");
    return m_impl->token;
}

const char* Token::getName() const
{
    return m_impl->token.getName();
}

std::ostream& operator<<(std::ostream &out, Token const& token)
{
    clang::Token const& tok = token.m_impl->token;
    if (tok.isLiteral())
    {
        out << std::string(tok.getLiteralData(), tok.getLength());
    }
    else if (tok.isAnyIdentifier())
    {
        clang::IdentifierInfo *i = tok.getIdentifierInfo();
        out << std::string(i->getNameStart(), i->getLength());
    }
    else
    {
        bool invalid = false;
        out << token.m_impl->preprocessor.getSpelling(tok, &invalid);
        if (invalid)
            out << "<invalid>";
    }
    return out;
}

}}

