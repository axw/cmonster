#ifndef _CSNAKE_PYTHON_TOKEN_ITERATOR_HPP
#define _CSNAKE_PYTHON_TOKEN_ITERATOR_HPP

namespace csnake {
namespace python {

// Python object structure to wrap a csnake::core::TokenIterator.
struct Preprocessor;
struct TokenIterator;

/**
 * Create a new heap-allocated TokenIterator from the specified preprocessor
 * object.
 */
TokenIterator* create_iterator(Preprocessor *preprocessor);

/**
 * Initialise the TokenIterator Python type object.
 */
PyObject* init_token_iterator_type();

}}

#endif

