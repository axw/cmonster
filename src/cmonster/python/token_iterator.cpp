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
#include <stdexcept>
#include <iostream>

#include "token_iterator.hpp"
#include "token.hpp"
#include "preprocessor.hpp"
#include "../core/token_iterator.hpp"

namespace cmonster {
namespace python {

static PyObject *TokenIteratorType = NULL;
PyDoc_STRVAR(TokenIterator_doc, "Token iterator objects");

struct TokenIterator
{
    PyObject_HEAD
    Preprocessor *preprocessor;
    cmonster::core::TokenIterator *iterator;
};

static void TokenIterator_dealloc(TokenIterator* self)
{
    if (self->iterator)
        delete self->iterator;
    Py_XDECREF(self->preprocessor);
    PyObject_Del((PyObject*)self);
}

static PyObject* TokenIterator_iter(TokenIterator *iter)
{
    Py_INCREF(iter);
    return (PyObject*)iter;
}

static PyObject* TokenIterator_iternext(TokenIterator *self)
{
    if (self->iterator)
    {
        try
        {
            if (self->iterator->has_next())
            {
                cmonster::core::Token &token = self->iterator->next();
                return (PyObject*)create_token(self->preprocessor, token);
            }
            else
            {
                delete self->iterator;
                self->iterator = NULL;
            }
        }
        catch (std::exception const& e)
        {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        }
        catch (...)
        {
            PyErr_SetNone(PyExc_RuntimeError);
            return NULL;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

static PyType_Slot TokenIteratorTypeSlots[] =
{
    {Py_tp_dealloc,  (void*)TokenIterator_dealloc},
    {Py_tp_doc,      (void*)TokenIterator_doc},
    {Py_tp_iter,     (void*)TokenIterator_iter},
    {Py_tp_iternext, (void*)TokenIterator_iternext},
    {Py_tp_alloc,    (void*)PyType_GenericAlloc},
    {Py_tp_new,      (void*)PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec TokenIteratorTypeSpec =
{
    "cmonster._preprocessor.TokenIterator",
    sizeof(TokenIterator),
    0,
    Py_TPFLAGS_DEFAULT,
    TokenIteratorTypeSlots
};

///////////////////////////////////////////////////////////////////////////////

TokenIterator* create_iterator(Preprocessor *preprocessor)
{
    TokenIterator *iter = (TokenIterator*)PyObject_CallObject(
        (PyObject*)TokenIteratorType, NULL);
    if (iter)
    {
        try
        {
            iter->iterator = get_preprocessor(preprocessor).create_iterator();
        }
        catch (std::exception const& e)
        {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        }
        catch (...)
        {
            PyErr_SetNone(PyExc_RuntimeError);
            PyObject_Del(iter);
            return NULL;
        }
    }
    Py_INCREF(preprocessor);
    iter->preprocessor = preprocessor;
    return iter;
}

PyObject* init_token_iterator_type()
{
    TokenIteratorType = PyType_FromSpec(&TokenIteratorTypeSpec);
    if (!TokenIteratorType)
        return NULL;
    if (PyType_Ready((PyTypeObject*)TokenIteratorType) < 0)
        return NULL;
    return TokenIteratorType;
}

}}

