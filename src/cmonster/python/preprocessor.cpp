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
#include <fstream>
#include <stdexcept>
#include <iostream>

#include "function_macro.hpp"
#include "preprocessor.hpp"
#include "scoped_pyobject.hpp"
#include "token_iterator.hpp"
#include "token_predicate.hpp"
#include "token.hpp"

namespace cmonster {
namespace python {

static PyTypeObject *PreprocessorType = NULL;
PyDoc_STRVAR(Preprocessor_doc, "Preprocessor objects");

struct Preprocessor
{
    PyObject_HEAD
    cmonster::core::Preprocessor *preprocessor;
};

static PyObject* Preprocessor_iter(Preprocessor *pp)
{
    return (PyObject*)create_iterator(pp);
}

static void Preprocessor_dealloc(Preprocessor* self)
{
    if (self->preprocessor)
        delete self->preprocessor;
    PyObject_Del((PyObject*)self);
}

// TODO support initialising with a file object or a string.
static int
Preprocessor_init(Preprocessor *self, PyObject *args, PyObject *kwds)
{
    char *filename;
    PyObject *include_paths_ = NULL;
    if (!PyArg_ParseTuple(args, "sO", &filename, &include_paths_))
        return -1;

    try
    {
        // Get the include paths.
        std::vector<std::string> include_paths;
        if (!PySequence_Check(include_paths_))
        {
            PyErr_SetString(PyExc_TypeError,
                "expected sequence of string for second argument");
        }

        const Py_ssize_t seqlen = PySequence_Size(include_paths_);
        if (seqlen == -1)
            return -1;
        for (Py_ssize_t i = 0; i < seqlen; ++i)
        {
            ScopedPyObject path_ = PySequence_GetItem(include_paths_, i);
            if (PyUnicode_Check(path_))
            {
                PyObject *u8 = PyUnicode_AsUTF8String(path_);
                if (u8)
                {
                    char *u8_chars;
                    Py_ssize_t u8_size;
                    if (PyBytes_AsStringAndSize(u8, &u8_chars, &u8_size) == -1)
                    {
                        Py_DECREF(u8);
                        return -1;
                    }
                    else
                    {
                        include_paths.push_back(
                            std::string(u8_chars, u8_size));
                        Py_DECREF(u8);
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                PyErr_SetString(PyExc_TypeError,
                    "expected sequence of string for second argument");
                return -1;
            }
        }

        // Create a core preprocessor object.
        self->preprocessor =
            new cmonster::core::Preprocessor(filename, include_paths);

        return 0;
    }
    catch (std::exception const& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
    }
    return -1;
}

/*
static PyObject*
Preprocessor_add_include_path(Preprocessor* self, PyObject *args)
{
    char *path;
    PyObject *sysinclude = Py_True;
    if (!PyArg_ParseTuple(args, "s|O:add_include_path", &path, &sysinclude))
        return NULL;

    try
    {
        self->preprocessor->add_include_path(
            path, PyObject_IsTrue(sysinclude));
    }
    catch (std::exception const& e)
    {
        //PyErr_SetString(DatabaseError, e.what());
        return NULL;
    }
    catch (...)
    {
        //PyErr_SetString(DatabaseError, "Unknown error occurred");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}
*/

static PyObject* Preprocessor_define(Preprocessor* self, PyObject *args)
{
    PyObject *macro;
    PyObject *predefined = Py_False;
    if (!PyArg_ParseTuple(args, "O|O:define", &macro, &predefined))
        return NULL;

    try
    {
        // First check if it's a string. If so, convert it to UTF-8 and define
        // the macro as per usual.
        if (PyUnicode_Check(macro))
        {
            PyObject *utf8 = PyUnicode_AsUTF8String(macro);
            if (utf8)
            {
                const char *macro = PyBytes_AsString(utf8);
                try
                {
                    if (macro)
                    {
                        self->preprocessor->define(
                            macro, PyObject_IsTrue(predefined));
                    }
                    Py_DECREF(utf8);
                }
                catch (...)
                {
                    Py_DECREF(utf8);
                    throw;
                }
            }
        }
        else if (PyCallable_Check(macro))
        {
            const char *name = PyEval_GetFuncName(macro);
            boost::shared_ptr<cmonster::core::FunctionMacro>
                function(new cmonster::python::FunctionMacro(self, macro));
            self->preprocessor->define(name, function);
        }
        else
        {
            PyErr_SetString(PyExc_TypeError, "expected string or callable");
            return NULL;
        }
    }
    catch (std::exception const& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (...)
    {
        PyErr_SetString(PyExc_RuntimeError, "Unknown error occurred");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Preprocessor_add_pragma(Preprocessor* self, PyObject *args)
{
    const char *name = NULL;
    PyObject *handler = NULL;
    if (!PyArg_ParseTuple(args, "sO:add_pragma", &name, &handler))
        return NULL;

    try
    {
        if (PyCallable_Check(handler))
        {
            boost::shared_ptr<cmonster::core::FunctionMacro>
                function(new cmonster::python::FunctionMacro(self, handler));
            self->preprocessor->add_pragma(name, function);
        }
        else
        {
            PyErr_SetString(PyExc_TypeError, "expected callable for handler");
            return NULL;
        }
    }
    catch (std::exception const& e)
    {
        //PyErr_SetString(DatabaseError, e.what());
        return NULL;
    }
    catch (...)
    {
        //PyErr_SetString(DatabaseError, "Unknown error occurred");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Preprocessor_tokenize(Preprocessor* self, PyObject *args)
{
    const char *s = NULL;
    Py_ssize_t len;
    PyObject *expand = Py_False;
    if (!PyArg_ParseTuple(args, "s#|O:tokenize", &s, &len, &expand))
        return NULL;

    try
    {
        std::vector<cmonster::core::Token> result =
            self->preprocessor->tokenize(s, len, PyObject_IsTrue(expand));

        ScopedPyObject tuple(PyTuple_New(result.size()));
        if (!tuple)
            return NULL;

        for (Py_ssize_t i = 0; i < (Py_ssize_t)result.size(); ++i)
        {
            Token *token = create_token(self, result[i]);
            if (!token)
                return NULL;
            PyTuple_SetItem(tuple, i, (PyObject*)token);
        }
        return tuple.release();
    }
    catch (std::exception const& e)
    {
        //PyErr_SetString(DatabaseError, e.what());
        return NULL;
    }
    catch (...)
    {
        //PyErr_SetString(DatabaseError, "Unknown error occurred");
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Preprocessor_preprocess(Preprocessor* self, PyObject *args)
{
    long fd;
    if (!PyArg_ParseTuple(args, "l:preprocess", &fd))
        return NULL;

    self->preprocessor->preprocess(fd);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef Preprocessor_methods[] =
{
//    {(char*)"add_include_path",
//     (PyCFunction)&Preprocessor_add_include_path, METH_VARARGS},
    {(char*)"define",
     (PyCFunction)&Preprocessor_define, METH_VARARGS},
    {(char*)"add_pragma",
     (PyCFunction)&Preprocessor_add_pragma, METH_VARARGS},
    {(char*)"tokenize",
     (PyCFunction)&Preprocessor_tokenize, METH_VARARGS},
    {(char*)"preprocess",
     (PyCFunction)&Preprocessor_preprocess, METH_VARARGS},
    {NULL}
};

static PyType_Slot PreprocessorTypeSlots[] =
{
    {Py_tp_dealloc, (void*)Preprocessor_dealloc},
    {Py_tp_init,    (void*)Preprocessor_init},
    {Py_tp_methods, (void*)Preprocessor_methods},
    {Py_tp_doc,     (void*)Preprocessor_doc},
    {Py_tp_iter,    (void*)Preprocessor_iter},
    {Py_tp_alloc,   (void*)PyType_GenericAlloc},
    {Py_tp_new,     (void*)PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec PreprocessorTypeSpec =
{
    "cmonster._preprocessor.Preprocessor",
    sizeof(Preprocessor),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    PreprocessorTypeSlots
};

cmonster::core::Preprocessor& get_preprocessor(Preprocessor *wrapper)
{
    if (!wrapper) // XXX undefined behaviour?
        throw std::invalid_argument("wrapper == NULL");
    return *wrapper->preprocessor;
}

PyTypeObject* init_preprocessor_type()
{
    PreprocessorType = (PyTypeObject*)PyType_FromSpec(&PreprocessorTypeSpec);
    if (!PreprocessorType)
        return NULL;
    if (PyType_Ready((PyTypeObject*)PreprocessorType) < 0)
        return NULL;
    return PreprocessorType;
}

PyTypeObject* get_preprocessor_type()
{
    return PreprocessorType;
}

}}

