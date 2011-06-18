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

#include "preprocessor.hpp"
#include "token_iterator.hpp"
#include "function_macro.hpp"

#include <boost/unordered/unordered_map.hpp>
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>
#include <boost/wave/cpp_context.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>

///////////////////////////////////////////////////////////////////////////////

namespace {

typedef boost::wave::cpplexer::lex_iterator<
    csnake::core::token_type> lex_iterator_type;

/**
 * Reads in and stores the contents of the input stream provided. This is used
 * by PreprocessorImpl to pass to the Wave context base class.
 *
 * TODO Investigate the value of getting rid of this, in favour of a
 *      ForwardIterator for a file input stream (by seeking when necessary.)
 */
struct input_holder
{
    input_holder(std::istream &input_) : input(
        std::istreambuf_iterator<char>(input_.rdbuf()),
        std::istreambuf_iterator<char>()) {}
    std::string input;
};

struct csnake_preprocessor_hooks
  : public boost::wave::context_policies::default_preprocessing_hooks
{
    template <typename ContextT, typename TokenT,
              typename ContainerT, typename IteratorT>
    bool expanding_function_like_macro(
             ContextT const& context, TokenT const& macrodef,
             std::vector<TokenT> const& formal_args,
             ContainerT const& definition, TokenT const& macrocall,
             std::vector<ContainerT> const& arguments,
             IteratorT const &seqstart, IteratorT const &seqend)
    {
        const std::string name = macrodef.get_value().c_str();
        boost::unordered_map<
            std::string,
            boost::shared_ptr<csnake::core::FunctionMacro>
        >::iterator iter = functions.find(name);
        if (iter == functions.end())
            return false;

        // Extract the arguments.
        std::vector<csnake::core::token_type> args;
        const bool placeholder = (
            arguments.size() == 1 && arguments.front().size() == 1 &&
            (boost::wave::token_id)arguments.front().front() ==
                boost::wave::T_PLACEMARKER);
        if (!placeholder)
        {
            for (typename std::vector<ContainerT>::const_iterator
                     iter = arguments.begin(); iter != arguments.end(); ++iter)
            {
                std::copy(iter->begin(), iter->end(),
                    std::back_inserter(args));
            }
        }

        // Call the function.
        boost::shared_ptr<csnake::core::FunctionMacro> const& function =
            iter->second;
        std::vector<csnake::core::token_type> result = (*function)(args);

        // Indeed, I am a bad, bad man. Redefine the "definition" of the macro.
        ContainerT &output = const_cast<ContainerT&>(definition);
        output.clear();
        std::copy(result.begin(), result.end(), std::back_inserter(output));
        return false;
    }

    template <typename ContextT, typename ContainerT>
    bool interpret_pragma(ContextT const& ctx, ContainerT &pending,
                          typename ContextT::token_type const& option,
                          ContainerT const& values,
                          typename ContextT::token_type const& act_token)
    {
        std::string name = option.get_value().c_str();
        boost::unordered_map<
            std::string,
            boost::shared_ptr<csnake::core::FunctionMacro>
        >::iterator iter = pragmas.find(name);
        if (iter != pragmas.end())
        {
            std::vector<csnake::core::token_type>
                arguments(values.begin(), values.end());
            std::vector<csnake::core::token_type>
                replacement = (*iter->second)(arguments);
            if (!replacement.empty())
            {
                std::copy(replacement.begin(), replacement.end(),
                    std::back_inserter(pending));
            }
            return true;
        }
        return false;
    }

    /**
     * Mapping from name to function, for dynamic macro replacement.
     */
    boost::unordered_map<
        std::string,
        boost::shared_ptr<csnake::core::FunctionMacro>
    > functions;

    /**
     * Mapping from name to pragma handler, for pragma interpretation.
     */
    boost::unordered_map<
        std::string,
        boost::shared_ptr<csnake::core::FunctionMacro>
    > pragmas;
};

} // anonymous namespace

///////////////////////////////////////////////////////////////////////////////

namespace csnake {
namespace core {

class PreprocessorImpl :
    private input_holder,
    public boost::wave::context<
        std::string::iterator,
        lex_iterator_type,
        boost::wave::iteration_context_policies::load_file_to_string,
        csnake_preprocessor_hooks,
        PreprocessorImpl
    >
{
public:
    PreprocessorImpl(std::istream &input_, const char *filename) :
      input_holder(input_),
      boost::wave::context<
        std::string::iterator,
        lex_iterator_type,
        boost::wave::iteration_context_policies::load_file_to_string,
        csnake_preprocessor_hooks,
        PreprocessorImpl
      >(input.begin(), input.end(), filename, csnake_preprocessor_hooks())
    {
        set_language(static_cast<boost::wave::language_support>(
            boost::wave::support_normal |
            boost::wave::support_c99 |
            boost::wave::support_option_emit_pragma_directives));
    }

    /**
     * Defines a variadic macro for the given name, and stores a function which
     * will be invoked for the replacement.
     */
    bool add_macro_function(std::string const& name,
                            boost::shared_ptr<FunctionMacro> const& function)
    {
        if (function)
        {
            if (add_macro_definition(name + "(...)="))
            {
                return get_hooks().functions.insert(
                    std::make_pair(name, function)).second;
            }
        }
        return false;
    }

    /**
     * Defines a Boost Wave pragma with the given name, and stores a function
     * which will be invoked for the replacement.
     */
    bool add_pragma(std::string const& name,
                    boost::shared_ptr<FunctionMacro> const& function)
    {
        if (function)
        {
            return get_hooks().pragmas.insert(
                std::make_pair(name, function)).second;
        }
        return false;
    }

private:
};

class TokenIteratorImpl : public TokenIterator
{
public:
    TokenIteratorImpl(PreprocessorImpl &preprocessor)
      : m_begin(preprocessor.begin()), m_end(preprocessor.end()) {}

    bool has_next() const throw()
    {
        if (m_begin == m_end)
            return false;
        return true;
    }

    token_type next()
    {
        return *m_begin++;
    }

private:
    PreprocessorImpl::iterator_type m_begin, m_end;
};

///////////////////////////////////////////////////////////////////////////////

Preprocessor::Preprocessor(boost::shared_ptr<std::istream> input,
                           const char *filename)
  : m_impl(new PreprocessorImpl(*input, filename)) {}

bool Preprocessor::add_include_path(std::string const& path, bool sysinclude)
{
    if (sysinclude)
        return m_impl->add_sysinclude_path(path.c_str());
    else
        return m_impl->add_include_path(path.c_str());
}

bool Preprocessor::define(std::string const& macro, bool predefined)
{
    return m_impl->add_macro_definition(macro, predefined);
}

bool Preprocessor::define(std::string const& name,
                          boost::shared_ptr<FunctionMacro> const& function)
{
    return m_impl->add_macro_function(name, function);
}

bool Preprocessor::add_pragma(std::string const& name,
                              boost::shared_ptr<FunctionMacro> const& handler)
{
    return m_impl->add_pragma(name, handler);
}

TokenIterator* Preprocessor::preprocess()
{
    return new TokenIteratorImpl(*m_impl);
}

}}

