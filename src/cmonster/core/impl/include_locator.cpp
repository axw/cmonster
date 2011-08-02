
struct MyDiagnosticClient : public clang::DiagnosticClient
{
    MyDiagnosticClient(clang::Preprocessor &pp,
                       clang::DiagnosticClient *delegate)
      : m_pp(pp), m_delegate(delegate) {}
    void HandleDiagnostic(clang::Diagnostic::Level level,
                          const clang::DiagnosticInfo &info)
    {
        if (info.getID() == clang::diag::err_pp_file_not_found)
        {
            clang::SourceManager &sm = m_pp.getSourceManager();
            clang::SourceLocation loc = info.getLocation();
            if (loc.isMacroID())
                loc = sm.getSpellingLoc(loc);

            llvm::SmallString<128> buffer;
            llvm::StringRef spelling = m_pp.getSpelling(loc, buffer);
            assert(!spelling.empty() &&
                   "Failed to resolve #include filename spelling");

            const bool angled = spelling[0] == '<';
            std::string const& filename = info.getArgStdStr(0);
            // TODO try external resolution

        }

        // If we get here, then the diagnostic is unhandled. Let's pass it on
        // to the delegate (if any).
        if (m_delegate.get())
            m_delegate->HandleDiagnostic(level, info);
        else
            clang::DiagnosticClient::HandleDiagnostic(level, info);
    }

private:
    clang::Preprocessor                    &m_pp;
    std::auto_ptr<clang::DiagnosticClient>  m_delegate;
};
