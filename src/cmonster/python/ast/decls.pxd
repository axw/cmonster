cimport clangtypes

cdef extern from "string":
    cdef cppclass string:
        char* c_str()

cdef extern from "clang/AST/DeclarationName.h":
    cdef cppclass DeclarationNameInfo:
        string getAsString()

cdef extern from "clang/AST/DeclBase.h" namespace "clang::DeclContext":
    cdef cppclass decl_iterator:
        decl_iterator()
        decl_iterator(decl_iterator&)
        decl_iterator(Decl*)
        Decl* operator*()
        decl_iterator& operator++()
        decl_iterator operator++(int)
        bint operator!=(decl_iterator)

cdef extern from "clang/AST/DeclBase.h" namespace "clang::Decl":
    enum Kind:
        Function

cdef extern from "clang/AST/DeclBase.h" namespace "clang":
    cdef cppclass DeclContext:
        decl_iterator decls_begin()
        decl_iterator decls_end()

    cdef cppclass Decl:
        Kind getKind()
        char *getDeclKindName()
        DeclContext *getDeclContext()
        bint hasBody()

    cdef cppclass NamedDecl(Decl):
        #StringRef getName()
        string getNameAsString()

    cdef cppclass ValueDecl(NamedDecl):
        clangtypes.QualType getType()

    cdef cppclass DeclaratorDecl(ValueDecl):
        pass

    cdef cppclass VarDecl(DeclaratorDecl):
        pass

    cdef cppclass ParmVarDecl(Decl):
        pass

    cdef cppclass FunctionDecl(DeclaratorDecl, DeclContext):
        bint isVariadic()
        unsigned param_size()
        ParmVarDecl **param_begin()
        ParmVarDecl **param_end()

cdef extern from "clang/AST/Decl.h" namespace "clang":
    cdef cppclass TranslationUnitDecl(Decl, DeclContext):
        pass

