# vim: set filetype=pyrex:

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


###############################################################################
# Decl base class.

cdef class Decl:
    cdef clang.decls.Decl *ptr

    def __str__(self):
        return self.kind_name

    cdef DeclContext __getDeclContext(self):
        cdef DeclContext dc = DeclContext.__new__(DeclContext)
        dc.ptr = self.ptr.getDeclContext()
        assert dc.ptr != NULL
        return dc

    def __repr__(self):
        return "Decl(%s)" % self.kind_name

    property location:
        def __get__(self): return create_SourceLocation(self.ptr.getLocation())

    property kind:
        def __get__(self): return self.ptr.getKind()

    property kind_name:
        def __get__(self): return (<bytes>self.ptr.getDeclKindName()).decode()

    property body:
        def __get__(self):
            cdef clang.statements.Stmt *ptr = self.ptr.getBody()
            if ptr == NULL:
                return None
            stmt = create_Statement(ptr)
            assert type(stmt) is CompoundStatement
            return stmt.body

    property decl_context:
        def __get__(self): return self.__getDeclContext()


cdef Decl create_Decl(clang.decls.Decl *d):
    cdef Decl decl = {
        clang.decls.Function: FunctionDecl,
        clang.decls.ParmVar: ParmVarDecl,
    }.get(d.getKind(), Decl)()
    decl.ptr = d
    return decl

###############################################################################

cdef class NamedDecl(Decl):
    property name:
        def __get__(self):
            cdef bytes name = \
                (<clang.decls.ValueDecl*>self.ptr).getNameAsString().c_str()
            return name.decode()


cdef class ValueDecl(NamedDecl):
    property type:
        def __get__(self):
            return create_QualType(
                (<clang.decls.ValueDecl*>self.ptr).getType())


cdef class DeclaratorDecl(ValueDecl):
    pass


cdef class VarDecl(DeclaratorDecl):
    pass


cdef class ParmVarDecl(VarDecl):
    pass


###############################################################################
# TranslationUnitDecl

cdef class TranslationUnitDecl(Decl):
    cdef object parser

    def __init__(self, parser, capsule):
        assert PyCapsule_IsValid(capsule, <char*>0)
        cdef clang.decls.TranslationUnitDecl *tu = \
            <clang.decls.TranslationUnitDecl*>PyCapsule_GetPointer(
                capsule, <char*>0)
        self.parser = parser
        self.ptr = tu

    cdef DeclContext __getDeclContext(self):
        cdef DeclContext dc = DeclContext(self)
        cdef clang.decls.TranslationUnitDecl *tu = \
            <clang.decls.TranslationUnitDecl*>self.ptr
        dc.ptr = <clang.decls.DeclContext*>tu
        return dc

    property declarations:
        def __get__(self):
            return self.decl_context.declarations


###############################################################################

cdef class ParmVarDeclIterator:
    cdef clang.decls.ParmVarDecl **begin
    cdef clang.decls.ParmVarDecl **end
    def __next__(self):
        cdef clang.decls.ParmVarDecl *decl
        if self.begin != self.end:
            decl = deref(self.begin)
            inc(self.begin)
            return create_Decl(decl)
        raise StopIteration()


cdef class FunctionParameterList:
    cdef clang.decls.FunctionDecl *function
    def __len__(self):
        return self.function.param_size()
    def __iter__(self):
        cdef ParmVarDeclIterator iter_ = ParmVarDeclIterator()
        iter_.begin = self.function.param_begin()
        iter_.end = self.function.param_end()
        return iter_
    def __repr__(self):
        return repr([p for p in self])
    def __getitem__(self, i):
        if i < 0 or i >= len(self):
            raise IndexError("Parameter index out of range")
        return create_Decl(self.function.param_begin()[i])


cdef class FunctionDecl(DeclaratorDecl):
    property variadic:
        def __get__(self):
            return (<clang.decls.FunctionDecl*>self.ptr).isVariadic()

    property parameters:
        def __get__(self):
            cdef clang.decls.FunctionDecl *fd = \
                <clang.decls.FunctionDecl*>self.ptr
            cdef FunctionParameterList params = FunctionParameterList()
            params.function = <clang.decls.FunctionDecl*>self.ptr
            return params

