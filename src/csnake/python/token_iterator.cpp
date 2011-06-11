/* Define this to ensure only the limited API is used, so we can ensure forward
 * binary compatibility. */
#define Py_LIMITED_API

#include <Python.h>
#include <stdexcept>
#include <iostream>

#include <boost/wave/cpp_exceptions.hpp>

#include "token_iterator.hpp"
#include "token.hpp"
#include "preprocessor.hpp"
#include "../core/token_iterator.hpp"

namespace csnake {
namespace python {

static PyObject *TokenIteratorType = NULL;
PyDoc_STRVAR(TokenIterator_doc, "Token iterator objects");

struct TokenIterator
{
    PyObject_HEAD
    Preprocessor *preprocessor;
    csnake::core::TokenIterator *iterator;
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
                csnake::core::token_type token = self->iterator->next();
                return (PyObject*)create_token(token);
            }
            else
            {
                delete self->iterator;
                self->iterator = NULL;
            }
        }
        catch (boost::wave::preprocess_exception const& e)
        {
            // TODO Create PreprocessorException or similar.
            PyErr_SetString(PyExc_RuntimeError, e.description());
            return NULL;
        }
        catch (std::exception const& e)
        {
            PyErr_SetString(PyExc_RuntimeError, e.what());
            return NULL;
        }
        catch (...)
        {
            //PyErr_SetString(XError, e.what());
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
    "_preprocessor.TokenIterator",
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
            iter->iterator = get_preprocessor(preprocessor)->preprocess();
        }
        catch (std::exception const& e)
        {
            //PyErr_SetString(XError, e.what());
            return NULL;
        }
        catch (...)
        {
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

