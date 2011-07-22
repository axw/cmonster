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
#include <string>

#include "preprocessor.hpp"
#include "scoped_pyobject.hpp"
#include "token.hpp"
#include <iostream>
#include <sstream>

namespace cmonster {
namespace python {

static PyTypeObject *TokenType = NULL;
PyDoc_STRVAR(Token_doc, "Token objects");

struct Token
{
    PyObject_HEAD
    Preprocessor *preprocessor;
    cmonster::core::Token *token;
};

static void Token_dealloc(Token* self)
{
    Py_XDECREF(self->preprocessor);
    if (self->token)
        delete self->token;
    PyObject_Del((PyObject*)self);
}

Token* create_token(Preprocessor *pp, cmonster::core::Token const& value)
{
    ScopedPyObject args = Py_BuildValue("(O)", pp);
    Token *token = (Token*)PyObject_CallObject((PyObject*)TokenType, args);
    if (token)
        *token->token = value;
    return token;
}

static int Token_init(Token *self, PyObject *args, PyObject *kwargs)
{
    static const char *keywords[] = {"preprocessor", "id", "value", NULL};
    int token_kind = 0;
    Preprocessor *pp;
    PyObject *value = NULL;

    if (!PyArg_ParseTupleAndKeywords(
             args, kwargs, "O|iO", (char**)keywords, &pp, &token_kind, &value))
        return -1; 

    // Check the type of the preprocessor argument.
    if (PyObject_TypeCheck(pp, get_preprocessor_type()))
    {
        self->preprocessor = pp;
        Py_INCREF(self->preprocessor);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "a Preprocessor is required");
        return -1;
    }

    // Check token kind range.
    if (token_kind < 0 || token_kind >= clang::tok::NUM_TOKENS)
    {
        PyErr_SetString(PyExc_ValueError, "token kind is out of range");
        return -1;
    }

    // Convert the "value" object to a plain old C string, and create the
    // Token object.
    if (value)
    {
        PyObject *strobj = PyObject_Str(value);
        if (strobj)
        {
            PyObject *utf8 = PyUnicode_AsUTF8String(strobj);
            if (utf8)
            {
                char *u8_chars;
                Py_ssize_t u8_size;
                if (PyBytes_AsStringAndSize(utf8, &u8_chars, &u8_size) == -1)
                {
                    Py_DECREF(utf8);
                    return -1;
                }
                else
                {
                    self->token = get_preprocessor(pp).create_token(
                        static_cast<clang::tok::TokenKind>(token_kind),
                        u8_chars, u8_size);
                    Py_DECREF(utf8);
                }
            }
            Py_DECREF(strobj);
        }
    }

    // If a value was extracted, then self->token should be set. If not, let's
    // set it now with an empty value.
    if (!self->token)
    {
        self->token = get_preprocessor(pp).create_token(
            static_cast<clang::tok::TokenKind>(token_kind));
    }

    return 0;
}

static PyObject* Token_str(Token *self)
{
    std::ostringstream ss;
    ss << *self->token;
    std::string s = ss.str();
    return PyUnicode_FromStringAndSize(s.c_str(), s.size());
}

static PyObject* Token_repr(Token *self)
{
    std::ostringstream ss;
    ss << "Token(tok_" << self->token->getName() << ", '"
       << *self->token << "')";
    std::string s = ss.str();
    return PyUnicode_FromStringAndSize(s.c_str(), s.size());

/*
    cmonster::core::token_type::string_type const& value =
        self->token->get_value();
    cmonster::core::token_type::position_type const& position =
        self->token->get_position();
    return PyUnicode_FromFormat("Token(T_%s, '%s', %s:%u:%u)",
                                name.c_str(), value.c_str(),
                                position.get_file().c_str(),
                                position.get_line(), position.get_column());
*/
}

static PyObject* Token_get_token_id(Token *self, void *closure)
{
    clang::tok::TokenKind kind = self->token->getToken().getKind();
    return PyLong_FromLong(static_cast<long>(kind));
}

static PyObject*
Token_set_token_id(Token *self, PyObject *value, void *closure)
{
    long id = PyLong_AsLong(value);
    if (id == -1 && PyErr_Occurred())
        return NULL;
    if (id < 0 || id >= clang::tok::NUM_TOKENS)
    {
        PyErr_SetString(PyExc_ValueError, "token kind is out of range");
        return NULL;
    }
    self->token->getToken().setKind(static_cast<clang::tok::TokenKind>(id));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyGetSetDef Token_getset[] = {
    {(char*)"token_id", (getter)Token_get_token_id, (setter)Token_set_token_id,
     NULL /* docs */, NULL /* closure */},
    {NULL}
};

static PyType_Slot TokenTypeSlots[] =
{
    {Py_tp_dealloc, (void*)Token_dealloc},
    {Py_tp_getset,  (void*)Token_getset},
    {Py_tp_str,     (void*)Token_str},
    {Py_tp_repr,    (void*)Token_repr},
    {Py_tp_doc,     (void*)Token_doc},
    {Py_tp_init,    (void*)Token_init},
    {0, NULL}
};

static PyType_Spec TokenTypeSpec =
{
    "cmonster._preprocessor.Token",
    sizeof(Token),
    0,
    Py_TPFLAGS_DEFAULT,
    TokenTypeSlots
};

PyTypeObject* init_token_type()
{
    TokenType = (PyTypeObject*)PyType_FromSpec(&TokenTypeSpec);
    if (!TokenType)
        return NULL;
    if (PyType_Ready(TokenType) < 0)
        return NULL;
    return TokenType;
}

PyTypeObject* get_token_type()
{
    return TokenType;
}

cmonster::core::Token& get_token(Token *wrapper)
{
    if (!wrapper) // XXX undefined behaviour?
        throw std::invalid_argument("wrapper == NULL");
    return *wrapper->token;
}

}}

