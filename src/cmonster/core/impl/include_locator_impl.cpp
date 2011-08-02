#include "include_locator_impl.hpp"

#include <clang/Basic/FileManager.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/LexDiagnostic.h>

#include <iostream>

namespace cmonster {
namespace core {

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
                        m_include_fid = fid;
                        m_include_loc = loc;

                        // TODO Have a canonical definition of the pragma name
                        // somewhere (probably this class's header file). Maybe
                        // the pragma handler class should also live in this
                        // file.
                        //
                        // _Pragma("cmonster_include")
                        clang::Token *tok = new clang::Token[4];
                        tok[0].startToken();
                        tok[0].setKind(clang::tok::identifier);
                        m_pp.CreateString("_Pragma", 7, tok[0]);
                        tok[0].setIdentifierInfo(m_pp.getIdentifierInfo(
                            llvm::StringRef("_Pragma", 7)));
                        tok[1].startToken();
                        tok[1].setKind(clang::tok::l_paren);
                        m_pp.CreateString("(", 1, tok[1]);
                        tok[2].startToken();
                        tok[2].setKind(clang::tok::string_literal);
                        m_pp.CreateString("\"cmonster_include\"", 18, tok[2]);
                        tok[3].startToken();
                        tok[3].setKind(clang::tok::r_paren);
                        m_pp.CreateString(")", 1, tok[3]);
                        m_pp.EnterTokenStream(tok, 4, false, true);

                        // Finally, we must mark the "last diagnostic" as
                        // ignored so preprocessing continues as usual.
                        m_pp.getDiagnostics().setLastDiagnosticIgnored();
                        return;
                    }
                }
                // XXX Else... what? Maybe insert a comment back into
                // the preprocessor for problem determination?
            }
        }
        catch (...)
        {
            // Don't let exceptions escape to Clang.
            // TODO Push an error comment back into the preprocessor? Can't
            // raise a diagnostic from a diagnostic (Clang will barf).
        }
    }

    // If we get here, then the diagnostic is unhandled. Let's pass it on
    // to the delegate (if any).
    if (m_delegate.get())
        m_delegate->HandleDiagnostic(level, info);
    else
        clang::DiagnosticClient::HandleDiagnostic(level, info);
}

void IncludeLocatorDiagnosticClient::complete()
{
    if (!m_include_fid.isInvalid())
    {
        clang::FileID fid = m_include_fid;
        m_include_fid = clang::FileID();
        m_pp.EnterSourceFile(fid, m_pp.GetCurDirLookup(), m_include_loc);
    }
}

}}

