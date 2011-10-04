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

#include "../parser.hpp"
#include "parse_result_impl.hpp"
#include "preprocessor_impl.hpp"

#include <boost/scoped_ptr.hpp>

#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Parse/Parser.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaConsumer.h>
#include <llvm/Support/Host.h>

#include <iostream>

namespace cmonster {
namespace core {

class ParserImpl
{
public:
    ParserImpl(const char *buffer,
               size_t buflen,
               const char *filename) : m_compiler()
    {
        // Create diagnostics.
        m_compiler.createDiagnostics(0, NULL);

        // Create target info.
        // XXX make this configurable?
        clang::TargetOptions target_options;
        target_options.Triple = llvm::sys::getHostTriple();
        m_compiler.setTarget(clang::TargetInfo::CreateTargetInfo(
            m_compiler.getDiagnostics(), target_options));

        // Set the language options.
        // XXX make this configurable?
        clang::CompilerInvocation::setLangDefaults(
            m_compiler.getLangOpts(), clang::IK_CXX);

        // Configure the include paths.
        clang::HeaderSearchOptions &hsopts = m_compiler.getHeaderSearchOpts();
        hsopts.UseBuiltinIncludes = false;
        hsopts.UseStandardIncludes = false;
        hsopts.UseStandardCXXIncludes = false;

        // Disable predefined macros. We'll get these from the target
        // preprocessor.
        // FIXME disabling this causes a segfault, will come back to it.
        //compiler.getPreprocessorOpts().UsePredefines = false;

        // Create the rest.
        m_compiler.createFileManager();
        m_compiler.createSourceManager(m_compiler.getFileManager());

        // Set the main file.
        m_compiler.getSourceManager().createMainFileIDForMemBuffer(
            llvm::MemoryBuffer::getMemBufferCopy(
                llvm::StringRef(buffer, buflen), filename));

        m_preprocessor.reset(new impl::PreprocessorImpl(m_compiler));

        // Initialise parser and co.
        m_compiler.createASTContext();
        clang::SemaConsumer *semaConsumer = new clang::SemaConsumer;
        m_compiler.setASTConsumer(semaConsumer);
        m_compiler.createSema(clang::TU_Complete, NULL);
        semaConsumer->InitializeSema(m_compiler.getSema());
        m_parser.reset(new clang::Parser(
            m_compiler.getPreprocessor(), m_compiler.getSema()));
    }

    Preprocessor& getPreprocessor()
    {
        return *m_preprocessor;
    }

    ParseResult parse()
    {
        m_compiler.getPreprocessor().EnterMainSourceFile();
        m_parser->ParseTranslationUnit();
        m_preprocessor->check_exception();
        return ParseResult(boost::shared_ptr<ParseResultImpl>(
            new ParseResultImpl(m_compiler.getASTContext())));
    }

private:
    clang::CompilerInstance                   m_compiler;
    boost::scoped_ptr<impl::PreprocessorImpl> m_preprocessor;
    boost::scoped_ptr<clang::Parser>          m_parser;
};


Parser::Parser(const char *buffer,
               size_t buflen,
               const char *filename)
  : m_impl(new ParserImpl(buffer, buflen, filename))
{
}

Preprocessor& Parser::getPreprocessor()
{
    return m_impl->getPreprocessor();
}

ParseResult Parser::parse()
{
    return m_impl->parse();
}

}}

