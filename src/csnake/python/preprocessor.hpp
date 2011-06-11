#ifndef _CSNAKE_PYTHON_PREPROCESSOR_HPP
#define _CSNAKE_PYTHON_PREPROCESSOR_HPP

#include "../core/preprocessor.hpp"

namespace csnake {
namespace python {

// Python object structure to wrap a csnake::core::Preprocessor.
struct Preprocessor;

/**
 * Get the core preprocessor from the Python wrapper object.
 */
csnake::core::Preprocessor* get_preprocessor(Preprocessor *wrapper);

/**
 * Initialise the Preprocessor Python type object.
 */
PyObject* init_preprocessor_type();

}}

#endif

