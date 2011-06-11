#ifndef _CSNAKE_CORE_FUNCTIONMACRO_HPP
#define _CSNAKE_CORE_FUNCTIONMACRO_HPP

#include "token.hpp"
#include <vector>

namespace csnake {
namespace core {

/**
 * Abstract base class for function macros.
 */
class FunctionMacro
{
public:
    virtual ~FunctionMacro();

    /**
     * Invoke the function, given a (possibly empty) list of arguments
     * (tokens), and returning a (possibly empty) list of tokens.
     */
    virtual std::vector<token_type>
    operator()(std::vector<token_type> const& arguments) const = 0;
};

}}

#endif

