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
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "exception.hpp"
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
    try
    {
        return (PyObject*)create_iterator(pp);
    }
    catch (std::exception const& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
        return NULL;
    }
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
    catch (python_exception const& e)
    {
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
    PyObject *value = NULL;
    if (!PyArg_ParseTuple(args, "O|O:define", &macro, &value))
        return NULL;

    try
    {
        // First check if it's a string. If so, convert it to UTF-8 and define
        // an object-like macro.
        if (PyUnicode_Check(macro))
        {
            ScopedPyObject utf8_name(PyUnicode_AsUTF8String(macro));
            const char *macro_name;
            if (utf8_name && (macro_name = PyBytes_AsString(utf8_name)))
            {
                if (value)
                {
                    if (PyUnicode_Check(value)) // define(name, value)
                    {
                        ScopedPyObject utf8_value(
                            PyUnicode_AsUTF8String(value));
                        const char *macro_value;
                        if (utf8_value &&
                            (macro_value = PyBytes_AsString(utf8_value)))
                        {
                            // Value specified:
                            //     "#define macro_name macro_value".
                            self->preprocessor->define(
                                macro_name, macro_value);
                        }
                        else
                        {
                            return NULL;
                        }
                    }
                    else if (PyCallable_Check(value)) // define(name, callable)
                    {
                        boost::shared_ptr<cmonster::core::FunctionMacro>
                            function(new cmonster::python::FunctionMacro(
                                self, value));
                        self->preprocessor->define(macro_name, function);
                    }
                    else
                    {
                        PyErr_SetString(PyExc_TypeError,
                            "expected string or callable for value argument");
                        return NULL;
                    }
                }
                else
                {
                    // No value specified: "#define macro_name".
                    self->preprocessor->define(macro_name);
                }
            }
            else
            {
                return NULL;
            }
        }
        else if (PyCallable_Check(macro)) // define(callable)
        {
            // TODO ensure "value" was not specified.
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
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
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
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Preprocessor_tokenize(Preprocessor* self, PyObject *args)
{
    const char *s = NULL;
    Py_ssize_t len;
    if (!PyArg_ParseTuple(args, "s#:tokenize", &s, &len))
        return NULL;

    try
    {
        std::vector<cmonster::core::Token> result =
            self->preprocessor->tokenize(s, len);

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
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
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
    try
    {
        self->preprocessor->preprocess(fd);
    }
    catch (std::exception const& e)
    {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        if (!PyErr_Occurred())
            PyErr_SetNone(PyExc_RuntimeError);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Preprocessor_next(Preprocessor* self, PyObject *args)
{
    PyObject *expand = Py_True;
    if (!PyArg_ParseTuple(args, "|O:next", &expand))
        return NULL;
    try
    {
        std::auto_ptr<cmonster::core::Token> token(
            self->preprocessor->next(PyObject_IsTrue(expand)));
        if (token.get())
        {
            return (PyObject*)create_token(self, *token);
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError,
                "Internal error: Preprocessor returned NULL");
            return NULL;
        }
    }
    catch (std::exception const& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
        return NULL;
    }
}

PyObject* Preprocessor_format_tokens(Preprocessor *self, PyObject *args)
{
    PyObject *tokens;
    if (!PyArg_ParseTuple(args, "O:format_tokens", &tokens))
        return NULL;

    ScopedPyObject iter(PyObject_GetIter(tokens));
    if (!iter)
        return NULL;

    try
    {
        // Iterate through the tokens, accumulating in a vector.
        std::vector<cmonster::core::Token> token_vector;
        for (;;)
        {
            ScopedPyObject item(PyIter_Next(iter));
            if (!item)
            {
                if (PyErr_Occurred())
                    return NULL;
                else
                    break;
            }
            if (!PyObject_TypeCheck(item, get_token_type()))
            {
                PyErr_SetString(PyExc_TypeError, "Expected sequence of tokens");
                return NULL;
            }
            token_vector.push_back(get_token((Token*)(PyObject*)item));
        }

        // Format the sequence of tokens.
        std::ostringstream ss;
        if (!token_vector.empty())
            self->preprocessor->format(ss, token_vector);
        std::string formatted = ss.str();
        return PyUnicode_FromStringAndSize(formatted.data(), formatted.size());
    }
    catch (std::exception const& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    catch (python_exception const& e)
    {
        return NULL;
    }
    catch (...)
    {
        PyErr_SetNone(PyExc_RuntimeError);
        return NULL;
    }
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
    {(char*)"next",
     (PyCFunction)&Preprocessor_next, METH_VARARGS},
    {(char*)"format_tokens",
     (PyCFunction)&Preprocessor_format_tokens, METH_VARARGS},
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
    if (!wrapper)
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

