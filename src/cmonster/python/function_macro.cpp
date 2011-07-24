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

#include "function_macro.hpp"
#include "preprocessor.hpp"
#include "scoped_pyobject.hpp"
#include "token.hpp"

#include <stdexcept>

namespace {
    struct ScopedPyObject {
        ScopedPyObject(PyObject *ref) : m_ref(ref) {}
        ~ScopedPyObject() {Py_XDECREF(m_ref);}
        operator PyObject* () {return m_ref;}
        PyObject *m_ref;
    };
}

namespace cmonster {
namespace python {

FunctionMacro::FunctionMacro(Preprocessor *preprocessor, PyObject *callable)
  : m_preprocessor(preprocessor), m_callable(callable)
{
    if (!preprocessor)
        throw std::invalid_argument("preprocessor == NULL");
    if (!callable)
        throw std::invalid_argument("callable == NULL");
    Py_INCREF((PyObject*)m_preprocessor);
    Py_INCREF(m_callable);
}

FunctionMacro::~FunctionMacro()
{
    Py_DECREF(m_callable);
    Py_DECREF((PyObject*)m_preprocessor);
}

std::vector<cmonster::core::Token>
FunctionMacro::operator()(
    std::vector<cmonster::core::Token> const& arguments) const
{
    // Create the arguments tuple.
    ScopedPyObject args_tuple = PyTuple_New(arguments.size());
    if (!args_tuple)
        throw std::runtime_error("Failed to create argument tuple");
    for (Py_ssize_t i = 0; i < static_cast<Py_ssize_t>(arguments.size()); ++i)
    {
        Token *token = create_token(m_preprocessor, arguments[i]);
        PyTuple_SetItem(args_tuple, i, reinterpret_cast<PyObject*>(token));
    }

    // Call the function.
    ScopedPyObject py_result = PyObject_Call(m_callable, args_tuple, NULL);
    if (!py_result)
    {
        // TODO raise proper exception
        PyErr_Print();
        throw std::runtime_error("buhbow");
    }

    // Transform the result.
    std::vector<cmonster::core::Token> result;
    if (py_result == Py_None)
        return result;

    // Is it a string? If so, tokenize it.
    if (PyUnicode_Check(py_result))
    {
        ScopedPyObject utf8(PyUnicode_AsUTF8String(py_result));
        if (utf8)
        {
            char *u8_chars;
            Py_ssize_t u8_size;
            if (PyBytes_AsStringAndSize(utf8, &u8_chars, &u8_size) == -1)
            {
                throw std::runtime_error("PyBytes_AsStringAndSize failed");
            }
            else
            {
                cmonster::core::Preprocessor &pp =
                    get_preprocessor(m_preprocessor);
                result = pp.tokenize(u8_chars, u8_size);
                return result;
            }
        }
        else
        {
            throw std::runtime_error("PyUnicode_AsUTF8String failed");
        }
    }

    // If it's not a string, it should be a sequence of Token objects.
    if (!PySequence_Check(py_result))
    {
        //PyErr_SetString(
        //    PyExc_ValueError,
        throw std::runtime_error(
            "macro functions must return a sequence of tokens");
        //return ;
    }

    const Py_ssize_t seqlen = PySequence_Size(py_result);
    if (seqlen == -1)
    {
        throw std::runtime_error("PySequence_Size failed");
    }
    else
    {
        for (Py_ssize_t i = 0; i < seqlen; ++i)
        {
            ScopedPyObject token_ = PySequence_GetItem(py_result, i);
            if (PyObject_TypeCheck(token_, get_token_type()))
            {
                Token *token = (Token*)(PyObject*)token_;
                result.push_back(get_token(token));
            }
            else
            {
                // Invalid return value.
                // TODO flesh out exception description.
                throw std::runtime_error("Invalid return value");
            }
        }
    }
    return result;
}

}}

