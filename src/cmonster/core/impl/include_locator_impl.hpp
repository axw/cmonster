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

#ifndef _CMONSTER_CORE_INCLUDE_LOCATOR_IMPL_HPP
#define _CMONSTER_CORE_INCLUDE_LOCATOR_IMPL_HPP

#include "../include_locator.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Preprocessor.h>

#include <boost/shared_ptr.hpp>
#include <memory>

namespace cmonster {
namespace core {

class IncludeLocatorDiagnosticClient : public clang::DiagnosticClient
{
public:
    /**
     * Constructor for IncludeLocatorDiagnosticClient.
     *
     * @param pp The preprocessor to which this object is attached.
     * @param delegate A DiagnosticClient to delegate unhandled diagnostics to.
     */
    IncludeLocatorDiagnosticClient(clang::Preprocessor &pp,
                                   clang::DiagnosticClient *delegate = 0);

    /**
     * @param locator The locator that will be used for external include
     *                location.
     */
    void setIncludeLocator(boost::shared_ptr<IncludeLocator> const& locator);

    /**
     * Override for clang::DiagnosticClient::HandleDiagnostic.
     *
     * This will trap #include failures and provide a second chance attempt.
     */
    void HandleDiagnostic(clang::Diagnostic::Level level,
                          const clang::DiagnosticInfo &info);

private:
    boost::shared_ptr<IncludeLocator>       m_locator;
    clang::Preprocessor                    &m_pp;
    std::auto_ptr<clang::DiagnosticClient>  m_delegate;
    clang::FileID                           m_include_fid;
    clang::SourceLocation                   m_include_loc;
};

}}

#endif

