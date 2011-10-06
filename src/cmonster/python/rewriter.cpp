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
#include "parse_result.hpp"
#include "rewriter.hpp"
#include "scoped_pyobject.hpp"
#include "source_location.hpp"
#include "pyfile_ostream.hpp"

#include <clang/Rewrite/Rewriter.h>
#include <clang/Basic/SourceManager.h>

namespace cmonster {
namespace python {

static PyTypeObject *RewriterType = NULL;
PyDoc_STRVAR(Rewriter_doc, "Rewriter objects");

struct Rewriter
{
    PyObject_HEAD
    ParseResult *result;
    clang::Rewriter *rewriter;
};

static void Rewriter_dealloc(Rewriter* self)
{
    if (self->rewriter)
        delete self->rewriter;
    Py_DECREF(self->result);
    PyObject_Del((PyObject*)self);
}

static int
Rewriter_init(Rewriter *self, PyObject *args, PyObject *kwds)
{
    if (!PyArg_ParseTuple(args, "O", &self->result))
        return -1;
    if (!PyObject_TypeCheck(self->result, get_parse_result_type()))
    {
        PyErr_SetString(PyExc_TypeError,
            "Expected ParseResult as first argument");
        return -1;
    }
    Py_INCREF(self->result);

    cmonster::core::ParseResult &result_ = get_parse_result(self->result);
    clang::ASTContext &astctx = result_.getClangASTContext();
    self->rewriter = new clang::Rewriter(
        astctx.getSourceManager(), astctx.getLangOptions());
    return 0;
}

static bool get_source_location(PyObject *arg, clang::SourceLocation &result)
{
    if (PyObject_TypeCheck(arg, get_source_location_type()))
    {
        result = get_source_location((SourceLocation*)arg);
        return true;
    }
    else
    {
        long encoding = PyLong_AsLong(arg);
        if (encoding == -1 && PyErr_Occurred())
        {
            PyErr_Clear();
            if (PyObject_HasAttrString(arg, "location"))
            {
                ScopedPyObject location(
                    PyObject_GetAttrString(arg, "location"));
                if (!location)
                    return false;
                if (get_source_location(location, result))
                    return true;
            }
        }
        else
        {
            result = clang::SourceLocation::getFromRawEncoding(
                static_cast<unsigned>(encoding));
        }
    }

    PyErr_SetString(PyExc_TypeError,
        "Expected cmonster.SourceLocation, encoded source location, "
        "or object with valid 'location' attribute");
    return false;
}

static PyObject*
insert_text(Rewriter *self, PyObject *loc,
            const char *text, int text_size,
            PyObject *indent, bool insert_after)
{
    // Check if indentation is required for new lines.
    int indent_ = PyObject_IsTrue(indent);
    if (indent_ == -1)
        return NULL;

    clang::SourceLocation sloc;
    if (!get_source_location(loc, sloc))
        return NULL;

    if (sloc.isInvalid())
    {
        PyErr_SetString(PyExc_ValueError, "Invalid source location");
        return NULL;
    }

    // Ensure source location is in main file.
    cmonster::core::ParseResult &result_ = get_parse_result(self->result);
    clang::ASTContext &astctx = result_.getClangASTContext();
    if (!astctx.getSourceManager().isFromMainFile(sloc))
    {
        PyErr_SetString(PyExc_ValueError,
            "Source location is outside main file");
        return NULL;
    }

    llvm::StringRef insertion(text, text_size);
    self->rewriter->InsertText(sloc, insertion, insert_after, indent_==1);
    Py_RETURN_NONE;
}

static PyObject*
Rewriter_insert_after(Rewriter *self, PyObject *args, PyObject *kw)
{
    PyObject *loc;
    const char *text;
    int text_size;
    PyObject *indent = Py_True;
    static const char *keywords[] = {"location", "text", "indent", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kw, "Os#|O:insert_before",
                                     (char**)keywords, &loc, &text,
                                     &text_size, &indent))
        return NULL;
    return insert_text(self, loc, text, text_size, indent, true);
}

static PyObject*
Rewriter_insert_before(Rewriter *self, PyObject *args, PyObject *kw)
{
    PyObject *loc;
    const char *text;
    int text_size;
    PyObject *indent = Py_True;
    static const char *keywords[] = {"location", "text", "indent", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kw, "Os#|O:insert_before",
                                     (char**)keywords, &loc, &text,
                                     &text_size, &indent))
        return NULL;
    return insert_text(self, loc, text, text_size, indent, false);
}

// XXX should we support rewriting non-main files?
static PyObject* Rewriter_dump(Rewriter *self, PyObject *args)
{
    PyObject *file;
    if (!PyArg_ParseTuple(args, "O:dump", &file))
        return NULL;

    const clang::RewriteBuffer *buffer =
        self->rewriter->getRewriteBufferFor(
            self->rewriter->getSourceMgr().getMainFileID());
    if (buffer)
    {
        pyfile_ostream ostream(file);
        buffer->write(ostream);
        ostream.flush();
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyMethodDef Rewriter_methods[] =
{
    {(char*)"insert_after",
     (PyCFunction)&Rewriter_insert_after, METH_VARARGS | METH_KEYWORDS},
    {(char*)"insert_before",
     (PyCFunction)&Rewriter_insert_before, METH_VARARGS | METH_KEYWORDS},
    {(char*)"dump",
     (PyCFunction)&Rewriter_dump, METH_VARARGS},
    {NULL}
};

static PyType_Slot RewriterTypeSlots[] =
{
    {Py_tp_dealloc, (void*)Rewriter_dealloc},
    {Py_tp_init,    (void*)Rewriter_init},
    {Py_tp_methods, (void*)Rewriter_methods},
    {Py_tp_doc,     (void*)Rewriter_doc},
    {Py_tp_alloc,   (void*)PyType_GenericAlloc},
    {Py_tp_new,     (void*)PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec RewriterTypeSpec =
{
    "cmonster._cmonster.Rewriter",
    sizeof(Rewriter),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    RewriterTypeSlots
};

PyTypeObject* init_rewriter_type()
{
    RewriterType = (PyTypeObject*)PyType_FromSpec(&RewriterTypeSpec);
    if (!RewriterType)
        return NULL;
    if (PyType_Ready((PyTypeObject*)RewriterType) < 0)
        return NULL;
    return RewriterType;
}

PyTypeObject* get_rewriter_type()
{
    return RewriterType;
}

}}

