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

// XXX Py_LIMITED_API is disabled for now, see "init_source_location_type".
/* Define this to ensure only the limited API is used, so we can ensure forward
 * binary compatibility. */
//#define Py_LIMITED_API

#include <Python.h>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "exception.hpp"
#include "parse_result.hpp"
#include "source_location.hpp"
#include "scoped_pyobject.hpp"
#include "pyfile_ostream.hpp"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

namespace cmonster {
namespace python {

static PyTypeObject *SourceLocationType = NULL;
PyDoc_STRVAR(SourceLocation_doc, "SourceLocation objects");

struct SourceLocation
{
    PyObject_HEAD
    clang::SourceManager *source_manager;
    clang::SourceLocation source_location;
};

static void SourceLocation_dealloc(SourceLocation* self)
{
    if (self->source_manager)
        self->source_manager->Release();
    PyObject_Del((PyObject*)self);
}

const clang::SourceLocation& get_source_location(SourceLocation *loc)
{
    return loc->source_location;
}

SourceLocation*
create_source_location(
    clang::SourceLocation const& loc, clang::SourceManager &sm)
{
    SourceLocation *loc_ = (SourceLocation*)PyObject_CallFunction(
        (PyObject*)SourceLocationType, (char*)"(I)", loc.getRawEncoding());
    if (loc_)
    {
        sm.Retain();
        loc_->source_manager = &sm;
    }
    return loc_;
}

static int
SourceLocation_init(SourceLocation *self, PyObject *args, PyObject *kwds)
{
    unsigned int raw_encoding;
    PyObject *srcmgr = NULL;
    if (!PyArg_ParseTuple(args, "I|O", &raw_encoding, &srcmgr))
        return -1;

    if (srcmgr)
    {
        if (PyCapsule_IsValid(srcmgr, NULL))
        {
            void *ptr = PyCapsule_GetPointer(srcmgr, NULL);
            if (!ptr)
                return -1;
            self->source_manager = (clang::SourceManager*)ptr;
            self->source_manager->Retain();
        }
        else
        {
            PyErr_SetString(PyExc_TypeError,
                "Expected a capsule as first argument");
            return -1;
        }
    }

    self->source_location =
        clang::SourceLocation::getFromRawEncoding(raw_encoding);
    if (self->source_location.isInvalid())
    {
        PyErr_SetString(PyExc_ValueError, "Invalid source location");
        return -1;
    }

    return 0;
}

static PyObject*
SourceLocation_get_filename(SourceLocation *self, void *closure)
{
    clang::PresumedLoc ploc =
        self->source_manager->getPresumedLoc(self->source_location);
    return PyUnicode_FromString(ploc.getFilename());
}

static PyObject*
SourceLocation_get_in_main_file(SourceLocation *self, void *closure)
{
    if (self->source_manager->isFromMainFile(self->source_location))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject*
SourceLocation_get_line(SourceLocation *self, void *closure)
{
    const long line = self->source_manager->getPresumedLineNumber(
                          self->source_location);
    return PyLong_FromLong(line);
}

static PyObject*
SourceLocation_get_column(SourceLocation *self, void *closure)
{
    const long column = self->source_manager->getPresumedColumnNumber(
                            self->source_location);
    return PyLong_FromLong(column);
}

PyObject* SourceLocation_add(PyObject *lhs, PyObject *rhs)
{
    if (!PyObject_TypeCheck(lhs, SourceLocationType))
    {
        PyErr_SetString(PyExc_TypeError,
            "Expected SourceLocation as first argument");
        return NULL;
    }
    else
    {
        SourceLocation *self = (SourceLocation*)lhs;
        long offset = PyLong_AsLong(rhs);
        if (offset == -1 && PyErr_Occurred())
            return NULL;
        return (PyObject*)create_source_location(
            self->source_location.getLocWithOffset(offset),
            *self->source_manager);
    }
}

PyObject* SourceLocation_subtract(PyObject *lhs, PyObject *rhs)
{
    if (!PyObject_TypeCheck(lhs, SourceLocationType))
    {
        PyErr_SetString(PyExc_TypeError,
            "Expected SourceLocation as first argument");
        return NULL;
    }
    else
    {
        SourceLocation *self = (SourceLocation*)lhs;
        long offset = PyLong_AsLong(rhs);
        if (offset == -1 && PyErr_Occurred())
            return NULL;
        return (PyObject*)create_source_location(
            self->source_location.getLocWithOffset(-offset),
            *self->source_manager);
    }
}

// XXX we should create subclasses for file/macro locations, and put the
// relevant properties in each.
static PyGetSetDef SourceLocation_getset[] =
{
    {(char*)"filename", (getter)SourceLocation_get_filename,
     NULL, NULL /* docs */, NULL /* closure */},
    {(char*)"line", (getter)SourceLocation_get_line,
     NULL, NULL /* docs */, NULL /* closure */},
    {(char*)"column", (getter)SourceLocation_get_column,
     NULL, NULL /* docs */, NULL /* closure */},
    {(char*)"in_main_file", (getter)SourceLocation_get_in_main_file,
     NULL, NULL /* docs */, NULL /* closure */},
    {NULL}
};

static PyType_Slot SourceLocationTypeSlots[] =
{
    {Py_tp_dealloc, (void*)SourceLocation_dealloc},
    {Py_tp_init,    (void*)SourceLocation_init},
    {Py_tp_getset,  (void*)SourceLocation_getset},
    {Py_tp_doc,     (void*)SourceLocation_doc},
    {Py_tp_alloc,   (void*)PyType_GenericAlloc},
    {Py_tp_new,     (void*)PyType_GenericNew},

    {Py_nb_add, (void*)SourceLocation_add},
    {Py_nb_subtract, (void*)SourceLocation_subtract},

    {0, NULL}
};

static PyType_Spec SourceLocationTypeSpec =
{
    "cmonster._cmonster.SourceLocation",
    sizeof(SourceLocation),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    SourceLocationTypeSlots
};

PyTypeObject* init_source_location_type()
{
    SourceLocationType =
        (PyTypeObject*)PyType_FromSpec(&SourceLocationTypeSpec);
    if (!SourceLocationType)
        return NULL;

    // FIXME (CPython Issue 13115) -- see "token.cpp" for more info.
    SourceLocationType->tp_as_number =
        &((PyHeapTypeObject*)SourceLocationType)->as_number;

    if (PyType_Ready((PyTypeObject*)SourceLocationType) < 0)
        return NULL;
    return SourceLocationType;
}

PyTypeObject* get_source_location_type()
{
    return SourceLocationType;
}

}}

