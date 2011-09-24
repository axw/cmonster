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
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "exception.hpp"
#include "parser.hpp"
#include "parse_result.hpp"
#include "preprocessor.hpp"
#include "scoped_pyobject.hpp"

namespace cmonster {
namespace python {

static PyTypeObject *ParserType = NULL;
PyDoc_STRVAR(Parser_doc, "Parser objects");

struct Parser
{
    PyObject_HEAD
    cmonster::core::Parser *parser;
};

static void Parser_dealloc(Parser* self)
{
    if (self->parser)
        delete self->parser;
    PyObject_Del((PyObject*)self);
}

// TODO support initialising with a file object or a string.
static int
Parser_init(Parser *self, PyObject *args, PyObject *kwds)
{
    char *buffer;
    int buflen;
    char *filename;
    if (!PyArg_ParseTuple(args, "s#|s", &buffer, &buflen, &filename))
        return -1;

    try
    {
        // Create a core parser object.
        // TODO
        self->parser = new cmonster::core::Parser(
            buffer, buflen, filename ? filename : "");
        return 0;
    }
    catch (...)
    {
        set_python_exception();
    }
    return -1;
}

static PyObject* Parser_parse(Parser *self, PyObject *args)
{
    try
    {
        cmonster::core::ParseResult result = self->parser->parse();
        return (PyObject*)create_parse_result(self, result);
    }
    catch (...)
    {
        set_python_exception();
    }
    return NULL;
}

static PyMethodDef Parser_methods[] =
{
    {(char*)"parse", (PyCFunction)&Parser_parse, METH_VARARGS},
    {NULL}
};

static PyObject* Parser_get_preprocessor(Parser *self, void *closure)
{
    // The Python Preprocessor wrapper object is stateless (all state is
    // managed in the held C++/core object), so we can safely recreate a
    // wrapper each time. This is to avoid circular references.
    return (PyObject*)create_preprocessor(self);
}

static PyGetSetDef Parser_getset[] =
{
    {(char*)"preprocessor", (getter)Parser_get_preprocessor,
     NULL, NULL /* docs */, NULL /* closure */},
    {NULL}
};

static PyType_Slot ParserTypeSlots[] =
{
    {Py_tp_dealloc, (void*)Parser_dealloc},
    {Py_tp_init,    (void*)Parser_init},
    {Py_tp_getset,  (void*)Parser_getset},
    {Py_tp_methods, (void*)Parser_methods},
    {Py_tp_doc,     (void*)Parser_doc},
    {Py_tp_alloc,   (void*)PyType_GenericAlloc},
    {Py_tp_new,     (void*)PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec ParserTypeSpec =
{
    "cmonster._cmonster.Parser",
    sizeof(Parser),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    ParserTypeSlots
};

cmonster::core::Parser& get_parser(Parser *wrapper)
{
    if (!wrapper)
        throw std::invalid_argument("wrapper == NULL");
    return *wrapper->parser;
}

PyTypeObject* init_parser_type()
{
    ParserType = (PyTypeObject*)PyType_FromSpec(&ParserTypeSpec);
    if (!ParserType)
        return NULL;
    if (PyType_Ready((PyTypeObject*)ParserType) < 0)
        return NULL;
    return ParserType;
}

PyTypeObject* get_parser_type()
{
    return ParserType;
}

}}

