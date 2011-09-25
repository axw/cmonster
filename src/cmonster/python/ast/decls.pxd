# Copyright (c) 2011 Andrew Wilkins <axwalk@gmail.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cimport clangtypes
cimport source
cimport statements


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
        Var
        ParmVar


cdef extern from "clang/AST/DeclBase.h" namespace "clang":
    cdef cppclass DeclContext:
        decl_iterator decls_begin()
        decl_iterator decls_end()

    cdef cppclass Decl:
        source.SourceLocation getLocation()
        Kind getKind()
        char *getDeclKindName()
        DeclContext *getDeclContext()
        statements.Stmt* getBody()
        bint hasBody()

    cdef cppclass NamedDecl(Decl):
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

