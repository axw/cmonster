/* Define this to ensure only the limited API is used, so we can ensure forward
 * binary compatibility. */
#define Py_LIMITED_API

#include <Python.h>
#include <fstream>
#include <stdexcept>
#include <iostream>

#include "preprocessor.hpp"
#include "token_iterator.hpp"
#include "function_macro.hpp"

namespace csnake {
namespace python {

static PyObject *PreprocessorType = NULL;
PyDoc_STRVAR(Preprocessor_doc, "Preprocessor objects");

struct Preprocessor
{
    PyObject_HEAD
    csnake::core::Preprocessor *preprocessor;
};

static void Preprocessor_dealloc(Preprocessor* self)
{
    if (self->preprocessor)
        delete self->preprocessor;
    PyObject_Del((PyObject*)self);
}

static int
Preprocessor_init(Preprocessor *self, PyObject *args, PyObject *kwds)
{
    char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return -1;

    try
    {
        boost::shared_ptr<std::istream> fin(new std::ifstream(filename));
        if (!fin->good())
        {
            //PyErr_SetString(PyExc_IOError, "failed to open file");
            PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
            return -1;
        }
        self->preprocessor = new csnake::core::Preprocessor(fin, filename);
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

static PyObject*
Preprocessor_add_include_path(Preprocessor* self, PyObject *args)
{
    // TODO Allow passing bool to specify if sys include.
    char *path;
    bool sysinclude = true;
    if (!PyArg_ParseTuple(args, "s:add_include", &path))
        return NULL;

    try
    {
        self->preprocessor->add_include_path(path, sysinclude);
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

static PyObject* Preprocessor_define(Preprocessor* self, PyObject *args)
{
    PyObject *macro;
    if (!PyArg_ParseTuple(args, "O:define", &macro))
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
                        self->preprocessor->define(macro);
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
            boost::shared_ptr<csnake::core::FunctionMacro>
                function(new csnake::python::FunctionMacro(macro));
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
    return (PyObject*)create_iterator(self);
}

static PyMethodDef Preprocessor_methods[] =
{
    {(char*)"add_include_path",
     (PyCFunction)&Preprocessor_add_include_path, METH_VARARGS},
    {(char*)"define",
     (PyCFunction)&Preprocessor_define, METH_VARARGS},
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
    {Py_tp_alloc,   (void*)PyType_GenericAlloc},
    {Py_tp_new,     (void*)PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec PreprocessorTypeSpec =
{
    "_preprocessor.Preprocessor",
    sizeof(Preprocessor),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    PreprocessorTypeSlots
};

csnake::core::Preprocessor* get_preprocessor(Preprocessor *wrapper)
{
    return wrapper ? wrapper->preprocessor : NULL;
}

PyObject* init_preprocessor_type()
{
    PreprocessorType = PyType_FromSpec(&PreprocessorTypeSpec);
    if (!PreprocessorType)
        return NULL;
    if (PyType_Ready((PyTypeObject*)PreprocessorType) < 0)
        return NULL;
    return PreprocessorType;
}

}}

