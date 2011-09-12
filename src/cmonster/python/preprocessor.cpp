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

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "exception.hpp"
#include "function_macro.hpp"
#include "include_locator.hpp"
#include "parser.hpp"
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
    Parser *parser;
    cmonster::core::Preprocessor *preprocessor;
};

Preprocessor* create_preprocessor(Parser *parser)
{
    ScopedPyObject args = Py_BuildValue("(O)", parser);
    if (!args)
        return NULL;
    return (Preprocessor*)PyObject_CallObject(
        (PyObject*)PreprocessorType, args);
}

static PyObject* Preprocessor_iter(Preprocessor *pp)
{
    try
    {
        return (PyObject*)create_iterator(pp);
    }
    catch (...)
    {
        set_python_exception();
        return NULL;
    }
}

static void Preprocessor_dealloc(Preprocessor* self)
{
    Py_XDECREF(self->parser);
    PyObject_Del((PyObject*)self);
}

// TODO support initialising with a file object or a string.
static int
Preprocessor_init(Preprocessor *self, PyObject *args, PyObject *kwds)
{
    if (!PyArg_ParseTuple(args, "O", &self->parser))
        return -1;

    if (!PyObject_TypeCheck(self->parser, get_parser_type()))
    {
        PyErr_SetString(PyExc_TypeError, "expected Parser as argument");
        return -1;
    }

    Py_INCREF(self->parser);
    self->preprocessor = &get_parser(self->parser).getPreprocessor();
    return 0;
}

static PyObject*
Preprocessor_add_include_dir(Preprocessor* self, PyObject *args)
{
    char *path;
    PyObject *sysinclude = Py_True;
    if (!PyArg_ParseTuple(args, "s|O:add_include_dir", &path, &sysinclude))
        return NULL;

    try
    {
        self->preprocessor->add_include_dir(
            path, PyObject_IsTrue(sysinclude));
        Py_INCREF(Py_None);
        return Py_None;
    }
    catch (...)
    {
        set_python_exception();
    }
    return NULL;
}

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
    catch (...)
    {
        set_python_exception();
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
    catch (...)
    {
        set_python_exception();
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
    catch (...)
    {
        set_python_exception();
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* Preprocessor_preprocess(Preprocessor* self, PyObject *args)
{
    PyObject *f = NULL;
    if (!PyArg_ParseTuple(args, "|O:preprocess", &f))
        return NULL;
    try
    {
        boost::shared_ptr<FILE> file;
        long fd;
        if (!f || f == Py_None)
        {
            fd = fileno(stdout);
        }
        else if (PyUnicode_Check(f))
        {
            ScopedPyObject utf8_filename(PyUnicode_AsUTF8String(f));
            const char *filename;
            if (utf8_filename && (filename = PyBytes_AsString(utf8_filename)))
            {
                file.reset(fopen(filename, "w"), &fclose);
                fd = fileno(file.get());
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            // Assume it's a file-like object with a fileno() method.
            PyObject *pylong = PyObject_CallMethod(f, (char*)"fileno", NULL);
            if (!pylong || (fd = PyLong_AsLong(pylong)) == -1)
                return NULL;
        }
        self->preprocessor->preprocess(fd);
    }
    catch (...)
    {
        set_python_exception();
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
    catch (...)
    {
        set_python_exception();
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
    catch (...)
    {
        set_python_exception();
        return NULL;
    }
}

PyObject* Preprocessor_set_include_locator(Preprocessor *self, PyObject *args)
{
    PyObject *locator_;
    if (!PyArg_ParseTuple(args, "O:set_include_locator", &locator_))
        return NULL;

    if (!PyCallable_Check(locator_))
    {
        PyErr_SetString(PyExc_TypeError, "Expected a callable");
        return NULL;
    }

    try
    {
        boost::shared_ptr<cmonster::core::IncludeLocator> locator(
            new cmonster::python::IncludeLocator(locator_));
        self->preprocessor->set_include_locator(locator);
        Py_INCREF(Py_None);
        return Py_None;
    }
    catch (...)
    {
        set_python_exception();
        return NULL;
    }
}

static PyMethodDef Preprocessor_methods[] =
{
    {(char*)"add_include_dir",
     (PyCFunction)&Preprocessor_add_include_dir, METH_VARARGS},
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
    {(char*)"set_include_locator",
     (PyCFunction)&Preprocessor_set_include_locator, METH_VARARGS},
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
    "cmonster._cmonster.Preprocessor",
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

