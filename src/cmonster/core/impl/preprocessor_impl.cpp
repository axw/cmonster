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

#include "preprocessor_impl.hpp"
#include "../function_macro.hpp"
#include "../token_iterator.hpp"
#include "../token_predicate.hpp"
#include "../token.hpp"
#include "exception_diagnostic_client.hpp"
#include "include_locator_impl.hpp"

#include <clang/Frontend/Utils.h>
#include <clang/Basic/FileManager.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Pragma.h>

#include <boost/exception_ptr.hpp>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

namespace {

struct null_deleter {
    void operator()(const void *) {}
};

struct PreprocessorResetter
{
    PreprocessorResetter(
        clang::CompilerInstance &compiler, clang::Preprocessor &pp)
      : m_compiler(compiler), m_pp(pp) {}
    ~PreprocessorResetter() {m_compiler.setPreprocessor(&m_pp);}
private:
    clang::CompilerInstance &m_compiler;
    clang::Preprocessor &m_pp;
};

} // Anonymous namespace.

namespace cmonster {
namespace core {
namespace impl {

struct FileChangePPCallback : public clang::PPCallbacks
{
    FileChangePPCallback() : depth(0), location() {}
    void FileChanged(clang::SourceLocation Loc,
                     clang::PPCallbacks::FileChangeReason Reason,
                     clang::SrcMgr::CharacteristicKind FileType)
    {
        location = Loc;
        switch (Reason)
        {
            case clang::PPCallbacks::EnterFile: ++depth; break;
            case clang::PPCallbacks::ExitFile: --depth; break;
            default: break;
        }
    }
    unsigned int depth;
    clang::SourceLocation location;
};

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
        boost::shared_ptr<cmonster::core::FunctionMacro> const& function,
        boost::exception_ptr &exception)
      : clang::PragmaHandler(llvm::StringRef(name.c_str(), name.size())),
        m_token_saver(token_saver), m_function(function),
        m_exception(exception) {}

    void HandlePragma(clang::Preprocessor &PP,
                      clang::PragmaIntroducerKind Introducer,
                      clang::Token &FirstToken)
    {
        // Discard remaining directive tokens (there shouldn't be any before
        // 'eod').
        clang::Token token;
        for (PP.Lex(token); token.isNot(clang::tok::eod);)
             PP.Lex(token);

        // Add the result tokens back into the preprocessor.
        try
        {
            std::vector<cmonster::core::Token> result =
                (*m_function)(m_token_saver.tokens);
            if (!result.empty())
            {
                // Enter the results back into the preprocessor.
                clang::Token *tokens(new clang::Token[result.size()]);
                try
                {
                    tokens[0] = result[0].getClangToken();
                    for (size_t i = 1; i < result.size(); ++i)
                    {
                        tokens[i] = result[i].getClangToken();
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
            return;
        }
        catch (...)
        {
            // Clang is compiled without exception support, so we have to
            // resort to some nastiness here. Basically we need to insert an
            // eof token to force the preprocessor to exit. We'll also store
            // the error message so we can recover it later.
            m_exception = boost::current_exception();
        }

        // XXX should this be configurable? Allow user to just emit a
        // diagnostic?
        //
        // If we get here, an exception was caught. Let's tell the preprocessor
        // to stop.
        clang::Token tok;
        tok.startToken();
        tok.setKind(clang::tok::eof);
        PP.EnterTokenStream(&tok, 1, false, true);
    }

private:
    TokenSaverPragmaHandler                          &m_token_saver;
    boost::shared_ptr<cmonster::core::FunctionMacro>  m_function;
    boost::exception_ptr                             &m_exception;
};

///////////////////////////////////////////////////////////////////////////////

class TokenIteratorImpl : public TokenIterator
{
public:
    TokenIteratorImpl(clang::Preprocessor &pp, boost::exception_ptr &exception)
      : m_pp(pp), m_exception(exception), m_current(m_pp), m_next()
    {
        // Pinched from "clang/lib/Frontend/PrintPreprocessedOutput.cpp". Skip
        // tokens from the predefines buffer.
        const clang::SourceManager &sm = m_pp.getSourceManager();
        do
        {
            m_pp.Lex(m_next);
            if (m_next.is(clang::tok::eof) || !m_next.getLocation().isFileID())
                break;
            clang::PresumedLoc PLoc = sm.getPresumedLoc(m_next.getLocation());
            if (PLoc.isInvalid())
                break;
            if (strcmp(PLoc.getFilename(), "<built-in>") != 0)
                break;
        } while (true);
        if (m_exception)
            boost::rethrow_exception(m_exception);
    }

    bool has_next() const throw()
    {
        return m_next.isNot(clang::tok::eof);
    }

    Token& next()
    {
        m_current.setClangToken(m_next);
        m_pp.Lex(m_next);
        if (m_exception)
            boost::rethrow_exception(m_exception);
        return m_current;
    }

private:
    clang::Preprocessor  &m_pp;
    boost::exception_ptr &m_exception;
    Token                 m_current;
    clang::Token          m_next;
};


///////////////////////////////////////////////////////////////////////////////

PreprocessorImpl::PreprocessorImpl(clang::CompilerInstance &compiler)
  : m_compiler(compiler), m_exception()
{
    m_compiler.createPreprocessor();

    // Set the predefines on the preprocessor.
    std::string predefines = m_compiler.getPreprocessor().getPredefines();
    predefines.append(
        "#define _CMONSTER_PRAGMA(...) _Pragma(#__VA_ARGS__)");
    m_compiler.getPreprocessor().setPredefines(predefines);

    // Add the "token saver" pragma handler. This will be used to store the
    // varargs arguments. There may be a more elegant way to do that with
    // just one pragma, but I couldn't think of it.
    m_token_saver = new impl::TokenSaverPragmaHandler;
    m_compiler.getPreprocessor().AddPragmaHandler(m_token_saver);

    // Add preprocessing callbacks so we know when a file is entered or
    // exited.
    m_file_change_callback = new impl::FileChangePPCallback;
    m_compiler.getPreprocessor().addPPCallbacks(m_file_change_callback);

    // Set the include locator diagnostic client.
    clang::DiagnosticClient *orig_client =
        m_compiler.getDiagnostics().takeClient();
    m_include_locator = new IncludeLocatorDiagnosticClient(
        m_compiler.getPreprocessor(), orig_client);
    m_compiler.getDiagnostics().setClient(m_include_locator);

    // Tell the diagnostic client that we've entered a source file, or bad
    // things happen when diagnostics are reported.
    // XXX Is it important to call EndSourceFile? Will anything leak?
    // XXX Is it okay to call this here? The outer Parser will not yet have
    //     attached its AST/Sema observer.
    orig_client->BeginSourceFile(
        m_compiler.getLangOpts(), &m_compiler.getPreprocessor());
}

bool
PreprocessorImpl::add_include_dir(std::string const& path, bool sysinclude)
{
    clang::HeaderSearch &headers =
        m_compiler.getPreprocessor().getHeaderSearchInfo();
    clang::FileManager &filemgr = headers.getFileMgr();
    const clang::DirectoryEntry *entry =
        filemgr.getDirectory(llvm::StringRef(path.c_str(), path.size()));

    // Take a copy of the existing search paths, and add the new one. If
    // it's a system path, insert it in after "system_dir_end". If it's a
    // user path, simply add it to the end of the vector.
    std::vector<clang::DirectoryLookup> search_paths(
        headers.search_dir_begin(), headers.search_dir_end());
    // TODO make sure it's not already in the list.
    const unsigned int n_quoted = std::distance(
        headers.quoted_dir_begin(), headers.quoted_dir_end());
    const unsigned int n_angled = std::distance(
        headers.angled_dir_begin(), headers.angled_dir_end());
    if (sysinclude)
    {
        clang::DirectoryLookup lookup(
            entry, clang::SrcMgr::C_System, true, false);
        search_paths.insert(
            search_paths.begin() + (n_quoted + n_angled), lookup);
    }
    else
    {
        clang::DirectoryLookup lookup(
            entry, clang::SrcMgr::C_User, true, false);
        search_paths.push_back(lookup);
    }
    headers.SetSearchPaths(
        search_paths, n_quoted, n_quoted+n_angled, false);
    return true;
}

bool
PreprocessorImpl::define(std::string const& name, std::string const& value)
{
    // Tokenize the value.
    std::vector<cmonster::core::Token> value_tokens;
    if (!value.empty())
        value_tokens = tokenize(value.c_str(), value.size());

    // TODO move this to a utility function somewhere.
    // Check if it's a function or an object-like macro.
    std::vector<std::string> arg_names;
    if (!name.empty() && name[name.size()-1] == ')')
    {
        size_t lparen = name.find('(');
        if (lparen == std::string::npos)
        {
            boost::throw_exception(std::invalid_argument(
                "Name ends with ')', but has no matching '('"));
        }

        // Split the args by commas, stripping whiespace.
        size_t start = lparen + 1;
        const size_t rparen = name.size() - 1;
        while (start < rparen)
        {
            while (start < rparen && name[start] == ' ')
                ++start;
            if (name[start] == ',')
            {
                std::stringstream ss;
                ss << start;
                boost::throw_exception(std::invalid_argument(
                    "Expected character other than ',' in string '" + name +
                    "', column: " + ss.str()));
            }

            size_t comma = name.find(',', start+1);
            if (comma == std::string::npos)
            {
                size_t end = rparen - 1;
                while (end > start && name[end] == ' ')
                    --end;
                arg_names.push_back(name.substr(start, end-start+1));
                start = rparen;
            }
            else
            {
                size_t end = comma - 1;
                while (end > start && name[end] == ' ')
                    --end;
                arg_names.push_back(name.substr(start, end-start+1));
                start = comma + 1;
            }
        }
        return add_macro_definition(
            name.substr(0, lparen), value_tokens, arg_names, true);
    }
    else
    {
        return add_macro_definition(
            name, value_tokens, std::vector<std::string>(), false);
    }
}

/**
 * Defines a simple macro.
 */
bool
PreprocessorImpl::add_macro_definition(
    std::string const& name,
    std::vector<cmonster::core::Token> const& value_tokens,
    std::vector<std::string> const& args, bool is_function)
{
    clang::Preprocessor &pp = m_compiler.getPreprocessor();
    clang::IdentifierInfo *macro_identifier = pp.getIdentifierInfo(name);
    clang::MacroInfo *macro =
        pp.AllocateMacroInfo(clang::SourceLocation());

    // Set the function arguments.
    if (is_function)
    {
        macro->setIsFunctionLike();
        if (!args.empty())
        {
            unsigned int n_args = args.size();
            if (args.back() == "...")
            {
                macro->setIsC99Varargs();
                --n_args;
            }
            std::vector<clang::IdentifierInfo*>
                arg_identifiers(args.size());
            for (unsigned int i = 0; i < n_args; ++i)
                arg_identifiers[i] = pp.getIdentifierInfo(args[i]);
            if (macro->isC99Varargs())
                arg_identifiers.back() =
                    pp.getIdentifierInfo("__VA_ARGS__");
            macro->setArgumentList(
                &arg_identifiers[0], arg_identifiers.size(),
                pp.getPreprocessorAllocator());
        }
    }

    // Set the macro body.
    if (!value_tokens.empty())
    {
        for (std::vector<cmonster::core::Token>::const_iterator
                 iter = value_tokens.begin();
             iter != value_tokens.end(); ++iter)
            macro->AddTokenToBody(iter->getClangToken());
        macro->setDefinitionEndLoc(
            value_tokens.back().getClangToken().getLocation());
    }

    // Is there an existing macro which is different? Then don't define the
    // new one.
    clang::MacroInfo *existing_macro = pp.getMacroInfo(macro_identifier);
    if (existing_macro)
    {
        bool result = macro->isIdenticalTo(*existing_macro, pp);
        macro->Destroy();
        return result;
    }

    pp.setMacroInfo(macro_identifier, macro);
    return true;
}

bool PreprocessorImpl::define(std::string const& name,
                          boost::shared_ptr<FunctionMacro> const& function)
{
    if (function && add_pragma(name, function, true))
    {
        clang::Preprocessor &pp = m_compiler.getPreprocessor();

        // Get the IdentifierInfo for the macro name.
        clang::IdentifierInfo *macro_identifier =
            pp.getIdentifierInfo(name);

        // Make sure the macro isn't already defined?
        clang::MacroInfo *macro = pp.getMacroInfo(macro_identifier);
        if (macro)
        {
            boost::throw_exception(
                std::runtime_error("Macro already defined"));
        }

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
        return true;
    }
    return false;
}

bool PreprocessorImpl::add_pragma(std::string const& name,
                              boost::shared_ptr<FunctionMacro> const& function)
{
    return add_pragma(name, function, false);
}

bool PreprocessorImpl::add_pragma(std::string const& name,
                              boost::shared_ptr<FunctionMacro> const& function,
                              bool with_namespace)
{
    if (function)
    {
        if (with_namespace)
        {
            m_compiler.getPreprocessor().AddPragmaHandler(
                "cmonster", new DynamicPragmaHandler(
                    *m_token_saver, name, function, m_exception));
        }
        else
        {
            m_compiler.getPreprocessor().AddPragmaHandler(
                new DynamicPragmaHandler(
                    *m_token_saver, name, function, m_exception));
        }
        return true;
    }
    return false;
}

void PreprocessorImpl::preprocess(long fd)
{
    llvm::raw_fd_ostream out(fd, false);

    clang::PreprocessorOutputOptions opts;
    opts.ShowComments = 1;
    //opts.ShowMacroComments = 1;
    //opts.ShowMacros = 1;

    clang::DoPrintPreprocessedInput(
        m_compiler.getPreprocessor(), &out, opts);
    if (m_exception)
        boost::rethrow_exception(m_exception);
}

TokenIterator* PreprocessorImpl::create_iterator()
{
    // Start preprocessing.
    m_compiler.getPreprocessor().EnterMainSourceFile();

    // Set a new DiagnosticClient that stores exceptions.
    m_include_locator->setDelegate(
        new ExceptionDiagnosticClient(m_exception));

    // Return a TokenIterator.
    return new TokenIteratorImpl(m_compiler.getPreprocessor(), m_exception);
}

// XXX should we just be creating a new Lexer?
std::vector<cmonster::core::Token>
PreprocessorImpl::tokenize(const char *s, size_t len)
{
    std::vector<cmonster::core::Token> result;
    if (!s || !len)
        return result;

    // If the main preprocessor hasn't yet been entered, create a temporary
    // one to lex from.
    clang::Preprocessor &old_pp = m_compiler.getPreprocessor();
    PreprocessorResetter pp_resetter(m_compiler, old_pp);
    if (m_file_change_callback->depth == 0)
    {
        m_compiler.resetAndLeakPreprocessor();
        m_compiler.createPreprocessor();
        m_compiler.getPreprocessor().EnterMainSourceFile();
    }

    // Create a memory buffer.
    std::auto_ptr<llvm::MemoryBuffer> mem(
        llvm::MemoryBuffer::getMemBufferCopy(
            llvm::StringRef(s, len), "<generated>"));

    // Create a file ID and enter it into the preprocessor.
    clang::Preprocessor &pp = m_compiler.getPreprocessor();
    clang::SourceManager &srcmgr = m_compiler.getSourceManager();

    // Transfer ownership of the memory buffer to the source manager.
    clang::FileID fid = srcmgr.createFileIDForMemBuffer(mem.release());

    // Record the current include depth, and enter the file. We can use the
    // file change callback to (a) ensure the memory buffer file was entered,
    // and (b) to determine when to stop lexing.
    //
    // Unfortunately the use of a "file" means the preprocessor output is
    // littered with line markers. It would be nice if we could get rid of
    // them.
    const unsigned int old_depth = m_file_change_callback->depth;
    pp.EnterSourceFile(fid, pp.GetCurDirLookup(), clang::SourceLocation());

    // Did something go awry when trying to enter the file? Bail out.
    if ((&pp == &old_pp) && m_file_change_callback->depth <= old_depth)
        return result;

    // Lex until we leave the file. We peek the next token, and if it's
    // eof or from a different file, bail out.
    //
    // Note: I was originally using the file change PP callback's depth
    // tracking to handle this, but I ran into a problem. The "ExitFile"
    // callback is not invoked until the next token is lexed, so we would
    // have to backtrack once we've exited the file. I found that if I had
    // two macros, one calling the other, both using tokenize, the second
    // entry to tokenize would start lexing the backtracked tokens before
    // the tokens from the memory buffer.
    clang::Token tok;
    while (pp.LookAhead(0).isNot(clang::tok::eof) &&
           srcmgr.getFileID(pp.LookAhead(0).getLocation()) == fid)
    {
        pp.LexUnexpandedToken(tok);

        // If we've had to create another preprocessor, then make sure we
        // recreate the IdentifierInfo objects in the existing preprocessor.
        // Everything else (importantly, the source manager) is shared between
        // the two preprocessor objects.
        if (&pp != &old_pp && tok.isAnyIdentifier())
        {
            clang::IdentifierInfo *II = tok.getIdentifierInfo();
            if (II)
                tok.setIdentifierInfo(old_pp.getIdentifierInfo(II->getName()));
        }
        result.push_back(cmonster::core::Token(pp, tok));
    }
    return result;
}

Token* PreprocessorImpl::next(bool expand)
{
    clang::Preprocessor &pp = m_compiler.getPreprocessor();
    clang::Token tok;
    if (expand)
        pp.Lex(tok);
    else
        pp.LexUnexpandedToken(tok);
    if (m_exception)
        boost::rethrow_exception(m_exception);
    return new Token(m_compiler.getPreprocessor(), tok);
}

// XXX it would be nice to just use Clang's "DoPrintPreprocessedInput",
// but it forces us to "enter the main source file", which means we have
// to create a whole new preprocessor from scratch. That might be the
// way to go anyway... we'll see how we go.
std::ostream& PreprocessorImpl::format(
    std::ostream &out,
    std::vector<cmonster::core::Token> const& tokens) const
{
    unsigned int current_line = 0, current_column = 1;
    clang::SourceManager const& sm = m_compiler.getSourceManager();
    for (std::vector<cmonster::core::Token>::const_iterator
             iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        Token const& token = *iter;
        clang::SourceLocation loc = token.getClangToken().getLocation();
        clang::PresumedLoc ploc = sm.getPresumedLoc(loc);
        if (ploc.isValid())
        {
            unsigned int line = ploc.getLine();
            unsigned int column = ploc.getColumn();
            if (line > current_line)
            {
                // Lines are 1-based, so we use zero to mean that we haven't
                // yet processed any tokens.
                if (current_line > 0)
                {
                    out << std::string(line-current_line, '\n');
                    current_column = 1;
                }
                current_line = line;
            }
            if (column > current_column)
            {
                out << std::string(column-current_column, ' ');
                current_column = column;
            }
            out << token;
            // TODO verify exactly getLength() chars were written
            current_column += token.getClangToken().getLength();
        }
    }
    return out;
}

void
PreprocessorImpl::set_include_locator(
    boost::shared_ptr<IncludeLocator> const& locator)
{
    m_include_locator->setIncludeLocator(locator);
}

Token* PreprocessorImpl::create_token(clang::tok::TokenKind kind,
                                      const char *value, size_t value_len)
{
    return new Token(m_compiler.getPreprocessor(),
                     kind, value, value_len);
}

}}}

