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

#ifndef _CMONSTER_PYTHON_EXCEPTION_HPP
#define _CMONSTER_PYTHON_EXCEPTION_HPP

/* Define this to ensure only the limited API is used, so we can ensure forward
 * binary compatibility. */
#define Py_LIMITED_API
#include <Python.h>

#include <cassert>

namespace cmonster {
namespace python {

/**
 * An exception class that is thrown when a Python exception has occurred.
 *
 * This should not inherit from std::exception, as it should always be left to
 * percolate right up to the point where the extension meets Python.
 */
struct python_exception
{
    /**
     * Default constructor. This should be called when a Python exception has
     * occurred.
     */
    python_exception() {assert(PyErr_Occurred());}

    /**
     * This constructor may be called to set a Python exception.
     */
    python_exception(PyObject *type, const char *message = NULL)
    {
        assert(!PyErr_Occurred());
        if (message)
            PyErr_SetString(type, message);
        else
            PyErr_SetNone(type);
    }
};

}}

#endif

