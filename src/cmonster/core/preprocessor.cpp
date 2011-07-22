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

//#undef NDEBUG
//#include <assert.h>

#include "preprocessor.hpp"
#include "function_macro.hpp"
#include "token_iterator.hpp"
#include "token_predicate.hpp"
#include "token.hpp"

//#include <boost/unordered/unordered_map.hpp>
#include <boost/scoped_array.hpp>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/Utils.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Pragma.h>
#include <clang/Lex/ScratchBuffer.h>
#include <llvm/Support/Host.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

namespace {

/**
 * A clang PragmaHandler implementation that stores the pragma arguments. These
 * will later be consumed by a DynamicPragmaHandler.
 */
struct TokenSaverPragmaHandler : public clang::PragmaHandler
{
    TokenSaverPragmaHandler()
      : clang::PragmaHandler(llvm::StringRef("cmonster_pragma", 15)),
        tokens() {}
    void HandlePragma(clang::Preprocessor &PP,
                      clang::PragmaIntroducerKind Introducer,
                      clang::Token &FirstToken)
    {
        tokens.clear();
        clang::Token token;
        for (PP.Lex(token); token.isNot(clang::tok::eod); PP.Lex(token))
        {
            // TODO set the start of line and leading space tokens. We could
            // use PPCallbacks::MacroExpands to get the flags of the macro
            // name.
            tokens.push_back(cmonster::core::Token(PP, token));
        }
    }
    std::vector<cmonster::core::Token> tokens;
};

/**
 * A clang PragmaHandler implementation that takes the tokens saved by a
 * TokenSaverPragmaHandler and passes them to a cmonster FunctionMacro object.
 * The resultant tokens (if any) are fed back into the preprocessor.
 */
struct DynamicPragmaHandler : public clang::PragmaHandler
{
    DynamicPragmaHandler(
        TokenSaverPragmaHandler &token_saver,
        std::string const& name,
        boost::shared_ptr<cmonster::core::FunctionMacro> const& function)
      : clang::PragmaHandler(llvm::StringRef(name.c_str(), name.size())),
        m_token_saver(token_saver), m_function(function) {}

    void HandlePragma(clang::Preprocessor &PP,
                      clang::PragmaIntroducerKind Introducer,
                      clang::Token &FirstToken)
    {
        // Discard the tokens (there shouldn't be any before 'eod').
        clang::Token token;
        for (PP.Lex(token); token.isNot(clang::tok::eod);)
             PP.Lex(token);

        // Add the result tokens back into the preprocessor.
        std::vector<cmonster::core::Token> result =
            (*m_function)(m_token_saver.tokens);
        if (!result.empty())
        {
            // Enter the results back into the preprocessor.
            clang::Token *tokens(new clang::Token[result.size()]);
            try
            {
                tokens[0] = result[0].getToken();
                for (size_t i = 1; i < result.size(); ++i)
                {
                    tokens[i] = result[i].getToken();
                    tokens[i].setFlag(clang::Token::LeadingSpace);
                }
            }
            catch (...)
            {
                delete [] tokens;
                throw;
            }
            PP.EnterTokenStream(tokens, result.size(), false, true);
        }
    }

private:
    TokenSaverPragmaHandler                          &m_token_saver;
    boost::shared_ptr<cmonster::core::FunctionMacro>  m_function;
};

} // anonymous namespace

///////////////////////////////////////////////////////////////////////////////

namespace cmonster {
namespace core {

class PreprocessorImpl
{
public:
    PreprocessorImpl(const char *filename,
                     std::vector<std::string> const& includes)
      : compiler(), token_saver()
    {
        // Create diagnostics.
        compiler.createDiagnostics(0, NULL);

        // Create target info.
        // XXX make this configurable?
        clang::TargetOptions target_options;
        target_options.Triple = llvm::sys::getHostTriple();
        compiler.setTarget(clang::TargetInfo::CreateTargetInfo(
            compiler.getDiagnostics(), target_options));

        // Set the language options.
        compiler.getLangOpts().CPlusPlus = 1;

        // Configure the include paths.
        clang::HeaderSearchOptions &hsopts = compiler.getHeaderSearchOpts();
        for (std::vector<std::string>::const_iterator iter = includes.begin();
             iter != includes.end(); ++iter)
        {
            hsopts.AddPath(llvm::StringRef(iter->c_str(), iter->size()),
                clang::frontend::After, true, false, false);
        }

        // Create the rest.
        compiler.createFileManager();
        compiler.createSourceManager(compiler.getFileManager());

        // Set the main file.
        // XXX Can we use a stream?
        compiler.getSourceManager().createMainFileID(
            compiler.getFileManager().getFile(filename));
        compiler.createPreprocessor();

        // Set the predefines on the preprocessor.
        std::string predefines = compiler.getPreprocessor().getPredefines();
        predefines.append(
            "#define _CMONSTER_PRAGMA(...) _Pragma(#__VA_ARGS__)");
        compiler.getPreprocessor().setPredefines(predefines);

        // Add the "token saver" pragma handler. This will be used to store the
        // varargs arguments. There may be a more elegant way to do that with
        // just one pragma, but I couldn't think of it.
        token_saver = new TokenSaverPragmaHandler;
        compiler.getPreprocessor().AddPragmaHandler(token_saver);
    }

    /**
     * Defines a variadic macro for the given name, and stores a function which
     * will be invoked for the replacement.
     */
    bool add_macro_function(std::string const& name,
                            boost::shared_ptr<FunctionMacro> const& function)
    {
        if (function && add_pragma(name, function, true))
        {
            clang::Preprocessor &pp = compiler.getPreprocessor();

            // Get the IdentifierInfo for the macro name.
            clang::IdentifierInfo *macro_identifier = pp.getIdentifierInfo(
                llvm::StringRef(name.c_str(), name.size()));

            // Make sure the macro isn't already defined?
            clang::MacroInfo *macro = pp.getMacroInfo(macro_identifier);
            if (macro)
                throw std::runtime_error("Macro already defined");

            // _CMONSTER_PRAGMA(cmonster_pragma __VA_ARGS__)
            clang::Token t;
            {
                t.startToken();
                t.setKind(clang::tok::identifier);
                t.setIdentifierInfo(pp.getIdentifierInfo("_CMONSTER_PRAGMA"));
                pp.CreateString("_CMONSTER_PRAGMA", 16, t);

                // Allocate a new macro.
                macro = pp.AllocateMacroInfo(t.getLocation());
                // Make the macro function-like, and varargs-capable.
                macro->setIsFunctionLike();
                macro->setIsC99Varargs();
                clang::IdentifierInfo *va_args =
                    pp.getIdentifierInfo("__VA_ARGS__");
                macro->setArgumentList(&va_args, 1U,
                    pp.getPreprocessorAllocator());
                macro->AddTokenToBody(t);
            }
            {
                t.startToken();
                t.setKind(clang::tok::l_paren);
                pp.CreateString("(", 1, t);
                macro->AddTokenToBody(t);
            }
            {
                t.startToken();
                t.setKind(clang::tok::identifier);
                t.setIdentifierInfo(pp.getIdentifierInfo("cmonster_pragma"));
                pp.CreateString("cmonster_pragma", 15, t);
                macro->AddTokenToBody(t);
            }
            {
                t.startToken();
                t.setKind(clang::tok::identifier);
                t.setIdentifierInfo(pp.getIdentifierInfo("__VA_ARGS__"));
                pp.CreateString("__VA_ARGS__", 11, t);
                t.setFlag(clang::Token::LeadingSpace);
                macro->AddTokenToBody(t);
            }
            {
                t.startToken();
                t.setKind(clang::tok::r_paren);
                pp.CreateString(")", 1, t);
                macro->AddTokenToBody(t);
            }

            // _Pragma("cmonster" #name)
            {
                t.startToken();
                t.setKind(clang::tok::identifier);
                t.setIdentifierInfo(pp.getIdentifierInfo("_Pragma"));
                pp.CreateString("_Pragma", 7, t);
                macro->AddTokenToBody(t);
            }
            {
                t.startToken();
                t.setKind(clang::tok::l_paren);
                pp.CreateString("(", 1, t);
                macro->AddTokenToBody(t);
            }
            {
                std::string qualified_name = "\"cmonster " + name + "\"";
                t.startToken();
                t.setKind(clang::tok::string_literal);
                pp.CreateString(
                    qualified_name.c_str(), qualified_name.size(), t);
                macro->AddTokenToBody(t);
            }
            {
                t.startToken();
                t.setKind(clang::tok::r_paren);
                pp.CreateString(")", 1, t);
                macro->AddTokenToBody(t);
                macro->setDefinitionEndLoc(t.getLocation());
            }

            // Add the macro to the preprocessor.
            pp.setMacroInfo(macro_identifier, macro);
            //pp.DumpMacro(*macro);
            return true;
        }
        return false;
    }

    /**
     * Defines a Boost Wave pragma with the given name, and stores a function
     * which will be invoked for the replacement.
     */
    bool add_pragma(std::string const& name,
                    boost::shared_ptr<FunctionMacro> const& function,
                    bool with_namespace = false)
    {
        if (function)
        {
            if (with_namespace)
            {
                compiler.getPreprocessor().AddPragmaHandler(
                    "cmonster", new DynamicPragmaHandler(
                        *token_saver, name, function));
            }
            else
            {
                compiler.getPreprocessor().AddPragmaHandler(
                    new DynamicPragmaHandler(*token_saver, name, function));
            }
            return true;
        }
        return false;
    }

#if 0
    bool add_include_path(std::string const& path, bool sysinclude)
    {
        clang::HeaderSearch &headers =
            compiler.getPreprocessor().getHeaderSearchInfo();
        clang::FileManager &filemgr = headers.getFileMgr();
        const clang::DirectoryEntry *entry =
            filemgr.getDirectory(llvm::StringRef(path.c_str(), path.size()));

        return false;
        // TODO
        //if (sysinclude)
        //    return m_impl->add_sysinclude_path(path.c_str());
        //else
        //    return m_impl->add_include_path(path.c_str());
    }
#endif

    clang::CompilerInstance compiler;
    TokenSaverPragmaHandler *token_saver;
};

class TokenIteratorImpl : public TokenIterator
{
public:
    TokenIteratorImpl(clang::Preprocessor &pp)
      : m_pp(pp), m_current(pp), m_next()
    {
        m_pp.Lex(m_next);
    }

    bool has_next() const throw()
    {
        return m_next.isNot(clang::tok::eof);
    }

    Token& next()
    {
        m_current.setToken(m_next);
        m_pp.Lex(m_next);
        return m_current;
    }

private:
    clang::Preprocessor &m_pp;
    Token                m_current;
    clang::Token         m_next;
};


///////////////////////////////////////////////////////////////////////////////

Preprocessor::Preprocessor(const char *filename,
                           std::vector<std::string> const& include_paths)
  : m_impl(new PreprocessorImpl(filename, include_paths)) {}

#if 0
bool Preprocessor::add_include_path(std::string const& path, bool sysinclude)
{
    return m_impl->add_include_path(path, sysinclude);
}
#endif

bool Preprocessor::define(std::string const& macro, bool predefined)
{
    return false;
    // TODO
    //return m_impl->add_macro_definition(macro, predefined);
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

void
Preprocessor::skip_while(boost::shared_ptr<TokenPredicate> const& predicate)
{
    m_impl->skip_while(predicate);
}

void Preprocessor::preprocess(long fd)
{
    llvm::raw_fd_ostream out(fd, false);

    clang::PreprocessorOutputOptions opts;
    opts.ShowComments = 1;
    //opts.ShowMacroComments = 1;
    //opts.ShowMacros = 1;

    clang::DoPrintPreprocessedInput(
        m_impl->compiler.getPreprocessor(), &out, opts);
}

TokenIterator* Preprocessor::create_iterator()
{
    // Start preprocessing.
    m_impl->compiler.getPreprocessor().EnterMainSourceFile();
    return new TokenIteratorImpl(m_impl->compiler.getPreprocessor());
}

Token* Preprocessor::create_token(clang::tok::TokenKind kind,
                                  const char *value, size_t value_len)
{
    return new Token(m_impl->compiler.getPreprocessor(),
                     kind, value, value_len);
}

}}

