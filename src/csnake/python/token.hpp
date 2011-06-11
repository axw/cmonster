#ifndef _CSNAKE_PYTHON_TOKEN_HPP
#define _CSNAKE_PYTHON_TOKEN_HPP

#include "../core/token.hpp"

namespace csnake {
namespace python {

// Python object structure to wrap a token_type.
struct Token;

/**
 * Create a new heap-allocated Token.
 */
Token* create_token(csnake::core::token_type const& token_value);

/**
 * Get the core token value from the Python wrapper object.
 */
csnake::core::token_type get_token(Token *wrapper);

/**
 * Initialise the Token Python type object.
 */
PyTypeObject* init_token_type();

/**
 * Get the Token Python type object.
 *
 * init_token_type must be called before this function.
 */
PyTypeObject* get_token_type();

}}

#endif

