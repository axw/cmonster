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

#include "exception.hpp"
#include "scoped_pyobject.hpp"
#include "token_predicate.hpp"
#include "token.hpp"

#include <stdexcept>

namespace cmonster {
namespace python {

TokenPredicate::TokenPredicate(Preprocessor *preprocessor, PyObject *callable)
  : m_preprocessor(preprocessor), m_callable(callable)
{
    if (!preprocessor)
        throw std::invalid_argument("preprocessor == NULL");
    if (!callable)
        throw std::invalid_argument("callable == NULL");
    Py_INCREF(m_preprocessor);
    Py_INCREF(m_callable);
}

TokenPredicate::~TokenPredicate()
{
    Py_DECREF(m_callable);
    Py_DECREF(m_preprocessor);
}

bool TokenPredicate::operator()(cmonster::core::Token const& token) const
{
    // Create the arguments tuple.
    ScopedPyObject args_tuple =
        Py_BuildValue("(O)", create_token(m_preprocessor, token));
    if (!args_tuple)
        throw python_exception();

    // Call the function.
    ScopedPyObject result = PyObject_Call(m_callable, args_tuple, NULL);
    if (!result)
        throw python_exception();
    return PyObject_IsTrue(result);
}

}}

