#ifndef _CSNAKE_PYTHON_FUNCTIONMACRO_HPP
#define _CSNAKE_PYTHON_FUNCTIONMACRO_HPP

#include "../core/function_macro.hpp"

namespace csnake {
namespace python {

/**
 */
class FunctionMacro : public csnake::core::FunctionMacro
{
public:
    FunctionMacro(PyObject *callable);
    ~FunctionMacro();

    std::vector<csnake::core::token_type>
    operator()(std::vector<csnake::core::token_type> const& arguments) const;

private:
    PyObject *m_callable;
};

}}

#endif

