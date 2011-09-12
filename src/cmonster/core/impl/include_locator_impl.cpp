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

#include "include_locator_impl.hpp"

#include <clang/Basic/FileManager.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/LexDiagnostic.h>

#include <iostream>
#include <stdexcept>

#include <boost/exception_ptr.hpp>
#include <boost/exception/to_string.hpp>

namespace {
struct DiagnosticsResetter
{
    DiagnosticsResetter(clang::Preprocessor &pp, clang::Diagnostic &diag)
      : m_pp(pp), m_diag(diag) {}
    ~DiagnosticsResetter() {m_pp.setDiagnostics(m_diag);}
private:
    clang::Preprocessor &m_pp;
    clang::Diagnostic &m_diag;
};
}

namespace cmonster {
namespace core {
namespace impl {

IncludeLocatorDiagnosticClient::IncludeLocatorDiagnosticClient(
    clang::Preprocessor &pp, clang::DiagnosticClient *delegate)
  : m_locator(), m_pp(pp), m_delegate(delegate), m_include_fid(),
    m_include_loc() {}

void
IncludeLocatorDiagnosticClient::setIncludeLocator(
    boost::shared_ptr<IncludeLocator> const& locator)
{
    m_locator = locator;
}

void
IncludeLocatorDiagnosticClient::HandleDiagnostic(
    clang::Diagnostic::Level level, const clang::DiagnosticInfo &info)
{
    if (m_locator && info.getID() == clang::diag::err_pp_file_not_found)
    {
        clang::SourceManager &sm = m_pp.getSourceManager();
        clang::SourceLocation loc = info.getLocation();
        if (loc.isMacroID())
            loc = sm.getSpellingLoc(loc);

        // Get the filename from the diagnostic info arguments.
        std::string const& filename = info.getArgStdStr(0);

        // Determine whether the #include filename is angled or quoted.
        llvm::SmallString<128> buffer;
        llvm::StringRef spelling = m_pp.getSpelling(loc, buffer);
        assert(!spelling.empty() &&
               "Failed to resolve #include filename spelling");
        const bool angled = spelling[0] == '<';

        try
        {
            // Try external resolution using the locator.
            std::string include(filename.size() + 2, angled ? '<' : '"');
            include.replace(1, filename.size(), filename);
            if (angled) include[include.size()-1] = '>';

            std::string path;
            if (m_locator->locate(include, path))
            {
                // Enter the located file.
                clang::FileManager &fm = m_pp.getFileManager();
                const clang::FileEntry *file = fm.getFile(path);
                if (file)
                {
                    // Nabbed from "clang/lib/Lex/PPDirectives.cpp".
                    // XXX this should be user-specifiable, right?
                    clang::HeaderSearch &hs = m_pp.getHeaderSearchInfo();
                    clang::SrcMgr::CharacteristicKind file_characteristic =
                        std::max(hs.getFileDirFlavor(file),
                                 sm.getFileCharacteristic(loc));

                    clang::FileID fid = sm.createFileID(
                        file, loc, file_characteristic);
                    if (!fid.isInvalid())
                    {
                        // We set a temporary diagnostics object on the
                        // preprocessor in case the "EnterSourceFile" below
                        // causes another error. Diagnostics barf when another
                        // is reported while one is already "in flight".
                        clang::Diagnostic &old_diag = m_pp.getDiagnostics();
                        clang::Diagnostic temp_diag(
                            m_pp.getDiagnostics().getDiagnosticIDs(),
                            this, false);
                        temp_diag.setSourceManager(
                            &old_diag.getSourceManager());
                        m_pp.setDiagnostics(temp_diag);
                        DiagnosticsResetter resetter(m_pp, old_diag);
                        m_pp.EnterSourceFile(fid, m_pp.GetCurDirLookup(), loc);

                        // Finally, we must mark the "last diagnostic" as
                        // ignored so preprocessing continues as usual.
                        old_diag.setLastDiagnosticIgnored();
                        return;
                    }
                }

                // Returned file does not exist. Raise a new diagnostic with
                // the new filename.
                clang::Diagnostic &old_diag = m_pp.getDiagnostics();
                old_diag.setLastDiagnosticIgnored();
                clang::Diagnostic temp_diag(
                    m_pp.getDiagnostics().getDiagnosticIDs(), this, false);
                temp_diag.setSourceManager(&old_diag.getSourceManager());
                temp_diag.Report(info.getLocation(), info.getID()) << path;
                return;
            }
        }
        catch (...)
        {
            // Don't let exceptions escape to Clang. Raise a new diagnostic.
            clang::Diagnostic &old_diag = m_pp.getDiagnostics();
            old_diag.setLastDiagnosticIgnored();
            clang::Diagnostic temp_diag(
                m_pp.getDiagnostics().getDiagnosticIDs(), this, false);
            temp_diag.setSourceManager(&old_diag.getSourceManager());

            // Report the diagnostic.
            unsigned diagId = temp_diag.getCustomDiagID(
                clang::Diagnostic::Error,
                llvm::StringRef("Exception thrown in include locator: %0"));
            boost::exception_ptr const& e = boost::current_exception();
            if (e)
            {
                std::string const& what = boost::to_string(e);
                temp_diag.Report(info.getLocation(), diagId) << what;
            }
            else
            {
                temp_diag.Report(info.getLocation(), diagId)
                    << "unknown exception occurred";
            }
            return;
        }
    }

    // If we get here, then the diagnostic is unhandled. Let's pass it on
    // to the delegate (if any).
    if (m_delegate.get())
        m_delegate->HandleDiagnostic(level, info);
    else
        clang::DiagnosticClient::HandleDiagnostic(level, info);
}

clang::DiagnosticClient* IncludeLocatorDiagnosticClient::takeDelegate()
{
    return m_delegate.release();
}

void
IncludeLocatorDiagnosticClient::setDelegate(clang::DiagnosticClient *delegate)
{
    m_delegate.reset(delegate);
}

}}}

