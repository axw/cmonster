#ifndef _CSNAKE_CORE_PREPROCESSOR_ITERATOR_HPP
#define _CSNAKE_CORE_PREPROCESSOR_ITERATOR_HPP

#include "token.hpp"

namespace csnake {
namespace core {

/**
 * Iterator class, as returned by Preprocessor::preprocess().
 */
class TokenIterator
{
public:
    virtual ~TokenIterator();

    /**
     * Check if there are any more tokens to be returned.
     */
    virtual bool has_next() const throw() = 0;

    /**
     * Get the next token, subsequently incrementing the iterator.
     */
    virtual token_type next() = 0;
};

}}

#endif

