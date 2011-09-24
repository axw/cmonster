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
#include "scoped_pyobject.hpp"

namespace cmonster {
namespace python {

static PyTypeObject *TranslationUnitDeclType = NULL;
static PyTypeObject *ParseResultType = NULL;
PyDoc_STRVAR(ParseResult_doc, "ParseResult objects");

struct ParseResult
{
    PyObject_HEAD
    Parser *parser;
    cmonster::core::ParseResult *result;
};

static void ParseResult_dealloc(ParseResult* self)
{
    if (self->result)
        delete self->result;
    Py_DECREF(self->parser);
    PyObject_Del((PyObject*)self);
}

ParseResult*
create_parse_result(Parser *parser, cmonster::core::ParseResult const& result_)
{
    assert(parser);
    ParseResult *result = (ParseResult*)PyObject_CallFunction(
        (PyObject*)ParseResultType, (char*)"(O)", parser);
    if (result)
        result->result = new cmonster::core::ParseResult(result_);
    return result;
}

static int
ParseResult_init(ParseResult *self, PyObject *args, PyObject *kwds)
{
    if (!PyArg_ParseTuple(args, "O", &self->parser))
        return -1;
    if (!PyObject_TypeCheck(self->parser, get_parser_type()))
    {
        PyErr_SetString(PyExc_TypeError, "Expected Parser as first argument");
        return -1;
    }
    Py_INCREF(self->parser);
    return 0;
}

static PyMethodDef ParseResult_methods[] =
{
    {NULL}
};

static PyObject*
ParseResult_get_translation_unit(ParseResult *self, void *closure)
{
    if (!TranslationUnitDeclType)
    {
        ScopedPyObject ast_module(PyImport_ImportModule("cmonster._ast"));
        if (!ast_module)
            return NULL;
        TranslationUnitDeclType = (PyTypeObject*)PyObject_GetAttrString(
            ast_module, "TranslationUnitDecl");
        if (!TranslationUnitDeclType)
            return NULL;
        Py_INCREF(TranslationUnitDeclType);
    }
    clang::ASTContext &context(self->result->getClangASTContext());
    clang::TranslationUnitDecl *decl = context.getTranslationUnitDecl();
    ScopedPyObject capsule(PyCapsule_New(decl, NULL, NULL));
    if (!capsule)
        return NULL;
    return PyObject_CallFunction(
        (PyObject*)TranslationUnitDeclType, (char*)"(OO)",
            self->parser, capsule.get());
    return NULL;
}

static PyGetSetDef ParseResult_getset[] =
{
    {(char*)"translation_unit", (getter)ParseResult_get_translation_unit,
     NULL, NULL /* docs */, NULL /* closure */},
    {NULL}
};

static PyType_Slot ParseResultTypeSlots[] =
{
    {Py_tp_dealloc, (void*)ParseResult_dealloc},
    {Py_tp_init,    (void*)ParseResult_init},
    {Py_tp_getset,  (void*)ParseResult_getset},
    {Py_tp_methods, (void*)ParseResult_methods},
    {Py_tp_doc,     (void*)ParseResult_doc},
    {Py_tp_alloc,   (void*)PyType_GenericAlloc},
    {Py_tp_new,     (void*)PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec ParseResultTypeSpec =
{
    "cmonster._cmonster.ParseResult",
    sizeof(ParseResult),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    ParseResultTypeSlots
};

cmonster::core::ParseResult& get_parse_result(ParseResult *wrapper)
{
    if (!wrapper)
        throw std::invalid_argument("wrapper == NULL");
    return *wrapper->result;
}

PyTypeObject* init_parse_result_type()
{
    ParseResultType = (PyTypeObject*)PyType_FromSpec(&ParseResultTypeSpec);
    if (!ParseResultType)
        return NULL;
    if (PyType_Ready((PyTypeObject*)ParseResultType) < 0)
        return NULL;
    return ParseResultType;
}

PyTypeObject* get_parse_result_type()
{
    return ParseResultType;
}

}}

