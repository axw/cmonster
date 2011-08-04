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
#include "include_locator.hpp"

#include <stdexcept>

namespace cmonster {
namespace python {

IncludeLocator::IncludeLocator(PyObject *callable) : m_callable(callable)
{
    if (!callable)
        throw std::invalid_argument("callable == NULL");
    Py_INCREF(m_callable);
}

IncludeLocator::~IncludeLocator()
{
    Py_DECREF(m_callable);
}

bool
IncludeLocator::locate(std::string const& include, std::string &abs_path) const
{
    // FIXME The Python exceptions won't fly... probably should convert to C++
    // exceptions and discard them.

    // Call the function. Anything other than a string will be treated as the
    // function having failed to locate the include.
    ScopedPyObject result = PyObject_CallFunction(
        m_callable, (char*)"s#", include.data(), (int)include.size());
    if (!result)
        throw python_exception();
    if (PyUnicode_Check(result))
    {
        ScopedPyObject utf8_path(PyUnicode_AsUTF8String(result));
        char *path_;
        Py_ssize_t size;
        if (utf8_path &&
            (PyBytes_AsStringAndSize(utf8_path, &path_, &size) != -1))
        {
            abs_path = std::string(path_, size);
            return true;
        }
        else
        {
            throw python_exception();
        }
    }
    // TODO handle file-like object
    return false;
}

}}

