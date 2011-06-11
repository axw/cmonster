#ifndef _CSNAKE_CORE_TOKEN_HPP
#define _CSNAKE_CORE_TOKEN_HPP

#include <boost/wave/cpplexer/cpp_lex_token.hpp>

namespace csnake {
namespace core {

/**
 * Typedef the internal boost::wave token type.
 */
typedef boost::wave::cpplexer::lex_token<> token_type;

}}

#endif

