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

namespace csnake {
namespace python {

FunctionMacro::FunctionMacro(PyObject *callable) : m_callable(callable)
{
    if (!callable)
        throw std::invalid_argument("callable == NULL");
    Py_INCREF(m_callable);
}

FunctionMacro::~FunctionMacro()
{
    Py_DECREF(m_callable);
}

std::vector<csnake::core::token_type>
FunctionMacro::operator()(
    std::vector<csnake::core::token_type> const& arguments) const
{
    // Create the arguments tuple.
    ScopedPyObject args_tuple = PyTuple_New(arguments.size());
    if (!args_tuple)
        throw std::runtime_error("Failed to create argument tuple");
    for (Py_ssize_t i = 0; i < static_cast<Py_ssize_t>(arguments.size()); ++i)
    {
        Token *token = create_token(arguments[i]);
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
    std::vector<csnake::core::token_type> result;
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
            if (PyUnicode_Check(token_))
            {
                // TODO tokenise string.
                result.push_back(csnake::core::token_type(
                    boost::wave::T_STRINGLIT,
                    "TODO",
                    csnake::core::token_type::position_type("?")
                ));
            }
            else if (PyObject_TypeCheck(token_, get_token_type()))
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

