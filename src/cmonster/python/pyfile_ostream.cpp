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

#include "pyfile_ostream.hpp"

namespace cmonster {
namespace python {

pyfile_ostream::pyfile_ostream(PyObject *file)
  : llvm::raw_ostream(), m_file(file)
{
    Py_XINCREF(m_file);
}

pyfile_ostream::~pyfile_ostream()
{
    ScopedPyObject result(PyObject_CallMethod(m_file, (char*)"flush", NULL));
}

void pyfile_ostream::write_impl(const char *ptr, size_t size)
{
    ScopedPyObject result(PyObject_CallMethod(
        m_file, (char*)"write", (char*)"s#", ptr, (Py_ssize_t)size));
}

uint64_t pyfile_ostream::current_pos() const
{
    ScopedPyObject result(PyObject_CallMethod(m_file, (char*)"tell", NULL));
    if (!result)
        return 0;
    return static_cast<uint64_t>(PyLong_AsLongLong(result));
}

}}

