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

#ifndef _CSNAKE_PYTHON_TOKEN_HPP
#define _CSNAKE_PYTHON_TOKEN_HPP

#include "../core/token.hpp"

namespace cmonster {
namespace python {

// Forward declaration to Preprocessor, to which the Token is bound.
struct Preprocessor;

// Python object structure to wrap a token_type.
struct Token;

/**
 * Create a new heap-allocated Token.
 */
Token* create_token(Preprocessor *pp, cmonster::core::Token const& token);

/**
 * Get the core token value from the Python wrapper object.
 */
cmonster::core::Token& get_token(Token *wrapper);

/**
 * Initialise the Token Python type object.
 */
PyTypeObject* init_token_type();

/**
 * Get the Token Python type object.
 *
 * init_token_type must be called before this function.
 */
PyTypeObject* get_token_type();

}}

#endif

