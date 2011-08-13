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
#include <exception>
#include <string>

#include "scoped_pyobject.hpp"

#include <boost/exception/all.hpp>

namespace cmonster {
namespace python {

/**
 * An exception class that is thrown when a Python exception has occurred.
 */
struct python_exception : public std::exception
{
    /**
     * Default constructor. This should be called when a Python exception has
     * occurred.
     */
    inline python_exception() {assert(PyErr_Occurred());}

    /**
     * This constructor may be called to set a Python exception.
     */
    inline python_exception(PyObject *type, const char *message = NULL)
    {
        assert(!PyErr_Occurred());
        if (message)
            PyErr_SetString(type, message);
        else
            PyErr_SetNone(type);
    }

    /**
     * No-throw destructor.
     */
    inline ~python_exception() throw() {}

    /**
     * Convert the current Python exception to a string.
     *
     * XXX should this clear the current exception too?
     */
    inline const char *what() const throw()
    {
        if (m_what.empty() && PyErr_Occurred())
        {
            PyObject *exc, *val, *tb;
            PyErr_Fetch(&exc, &val, &tb);
            PyErr_NormalizeException(&exc, &val, &tb);
            ScopedPyObject str(PyObject_Str(val));
            if (str)
            {
                ScopedPyObject utf8_value(PyUnicode_AsUTF8String(str));
                const char *value;
                if (utf8_value && (value = PyBytes_AsString(utf8_value)))
                    m_what = value;
            }
            PyErr_Restore(exc, val, tb);
        }
        return m_what.c_str();
    }

    static inline void boost_throw_exception()
    {
        assert(PyErr_Occurred());

        PyObject *exc, *val, *tb;
        PyErr_Fetch(&exc, &val, &tb);
        assert(exc && val && tb);

        // Get the line number from the traceback.
        ScopedPyObject lineno(PyObject_GetAttrString(tb, "tb_lineno"));
        assert(lineno);

        // Get the frame from the traceback, and restore the exception.
        ScopedPyObject frame(PyObject_GetAttrString(tb, "tb_frame"));
        assert(frame.get());
        PyErr_Restore(exc, val, tb);

        // Get the code object from the traceback, so we can get the file and
        // function names.
        ScopedPyObject code(PyObject_GetAttrString(frame, "f_code"));
        assert(code.get());
        ScopedPyObject filename_(PyObject_GetAttrString(code, "co_filename"));
        assert(filename_.get());
        ScopedPyObject name_(PyObject_GetAttrString(code, "co_name"));
        assert(name_.get());

        // Get the file and function names.
        ScopedPyObject utf8_filename(PyUnicode_AsUTF8String(filename_));
        assert(utf8_filename.get());
        const char *filename = PyBytes_AsString(utf8_filename);
        assert(filename);
        ScopedPyObject utf8_name(PyUnicode_AsUTF8String(name_));
        assert(utf8_name.get());
        const char *name = PyBytes_AsString(utf8_name);
        assert(name);

        boost::throw_exception(boost::enable_error_info(python_exception())
            << boost::throw_function(name)
            << boost::throw_file(filename)
            << boost::throw_line(PyLong_AsLong(lineno)));
    }

private:
    mutable std::string m_what;
};

/**
 * This function will set a Python exception, using boost::current_exception if
 * available. If a Python exception is already set, then nothing is done.
 */
void set_python_exception();

}}

#endif

