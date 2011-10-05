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

/* Define this to ensure only the limited API is used, so we can ensure forward
 * binary compatibility. */
#define Py_LIMITED_API

#include <Python.h>

#include <iostream>

#include "parser.hpp"
#include "parse_result.hpp"
#include "preprocessor.hpp"
#include "rewriter.hpp"
#include "token_iterator.hpp"
#include "token.hpp"

#include <clang/Basic/TokenKinds.h>

static PyModuleDef cmonstermodule = {
    PyModuleDef_HEAD_INIT,
    "_cmonster",
    "Extension module to expose a native C++ parser/preprocessor.",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

extern "C" {
PyMODINIT_FUNC
PyInit__cmonster(void)
{
    PyObject *ParserType = (PyObject*)cmonster::python::init_parser_type();
    if (!ParserType)
        return NULL;
    PyObject *ParseResultType =
        (PyObject*)cmonster::python::init_parse_result_type();
    if (!ParseResultType)
        return NULL;
    if (!cmonster::python::init_preprocessor_type())
        return NULL;
    if (!cmonster::python::init_token_iterator_type())
        return NULL;
    PyObject *TokenType = (PyObject*)cmonster::python::init_token_type();
    if (!TokenType)
        return NULL;
    PyObject *RewriterType = (PyObject*)cmonster::python::init_rewriter_type();
    if (!RewriterType)
        return NULL;

    // Initialise module.
    PyObject *module = PyModule_Create(&cmonstermodule);
    if (!module)
        return NULL;

    // Add types.
    Py_INCREF(ParserType);
    Py_INCREF(ParseResultType);
    Py_INCREF(TokenType);
    Py_INCREF(RewriterType);
    PyModule_AddObject(module, "Parser", ParserType);
    PyModule_AddObject(module, "ParseResult", ParseResultType);
    PyModule_AddObject(module, "Token", TokenType);
    PyModule_AddObject(module, "Rewriter", RewriterType);

    // Add constants (token kinds).
    for (long i = 0; i < clang::tok::NUM_TOKENS; ++i)
    {
        std::string name("tok_");
        name.append(std::string(clang::tok::getTokenName(
            static_cast<clang::tok::TokenKind>(i))));
        PyModule_AddIntConstant(module, name.c_str(), i);
    }

    return module;
}
}

