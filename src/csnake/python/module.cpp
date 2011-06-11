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
#include "preprocessor.hpp"
#include "token_iterator.hpp"
#include "token.hpp"

static PyModuleDef preprocessormodule = {
    PyModuleDef_HEAD_INIT,
    "_preprocessor",
    "Extension module to expose a native C++ preprocessor.",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

extern "C" {
PyMODINIT_FUNC
PyInit__preprocessor(void)
{
    PyObject *PreprocessorType = csnake::python::init_preprocessor_type();
    if (!PreprocessorType)
        return NULL;
    if (!csnake::python::init_token_iterator_type())
        return NULL;
    PyObject *TokenType = (PyObject*)csnake::python::init_token_type();
    if (!TokenType)
        return NULL;

    // Initialise module.
    PyObject *module = PyModule_Create(&preprocessormodule);
    if (!module)
        return NULL;

    // Add types.
    Py_INCREF(PreprocessorType);
    Py_INCREF(TokenType);
    PyModule_AddObject(module, "Preprocessor", PreprocessorType);
    PyModule_AddObject(module, "Token", TokenType);

    // Add constants (token IDs).
    for (int i = boost::wave::T_FIRST_TOKEN;
         i < boost::wave::T_LAST_TOKEN; ++i)
    {
        BOOST_WAVE_STRINGTYPE name("T_");
        name.append(boost::wave::get_token_name((boost::wave::token_id)i));
        PyModule_AddIntConstant(module, name.c_str(), i);
    }

    return module;
}
}

