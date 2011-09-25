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

from cpython.pycapsule cimport PyCapsule_IsValid, PyCapsule_GetPointer
from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc

cimport clangtypes
cimport decls
cimport source
cimport statements

###############################################################################

cdef class SourceLocation:
    cdef source.SourceLocation *ptr
    def __dealloc__(self):
        if self.ptr: del self.ptr

cdef SourceLocation create_SourceLocation(source.SourceLocation loc):
    cdef SourceLocation sl = SourceLocation()
    sl.ptr = new source.SourceLocation(loc)
    return sl

###############################################################################

cdef class Statement:
    cdef statements.Stmt *ptr
    property class_name:
        def __get__(self):
            return (<bytes>self.ptr.getStmtClassName()).decode()

cdef Statement create_Statement(statements.Stmt *ptr):
    cdef Statement stmt = Statement()
    stmt.ptr = ptr
    return stmt

###############################################################################

cdef class Decl:
    cdef decls.Decl *ptr

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
            cdef statements.Stmt *ptr = self.ptr.getBody()
            if ptr == NULL:
                return None
            return create_Statement(ptr)

    property decl_context:
        def __get__(self): return self.__getDeclContext()


cdef Decl create_Decl(decls.Decl *d):
    cdef Decl decl = {
        decls.Function: FunctionDecl,
        decls.ParmVar: ParmVarDecl,
    }.get(d.getKind(), Decl)()
    decl.ptr = d
    return decl


###############################################################################


cdef class Type:
    cdef clangtypes.const_Type_ptr ptr
    def __repr__(self):
        return "Type(%s)" % str(self)
    def __str__(self):
        return (<bytes>self.ptr.getTypeClassName()).decode()
    property builtin:
        def __get__(self): return self.ptr.isBuiltinType()


cdef class BuiltinType(Type):
    # Unfortunately, we can't directly access "cdef enum"'s from Python. So
    # long, DRY.
    Void = clangtypes.Void
    Bool = clangtypes.Bool
    Char_U = clangtypes.Char_U
    UChar = clangtypes.UChar
    WChar_U = clangtypes.WChar_U
    Char16 = clangtypes.Char16
    Char32 = clangtypes.Char32
    UShort = clangtypes.UShort
    UInt = clangtypes.UInt
    ULong = clangtypes.ULong
    ULongLong = clangtypes.ULongLong
    UInt128 = clangtypes.UInt128
    Char_S = clangtypes.Char_S
    SChar = clangtypes.SChar
    WChar_S = clangtypes.WChar_S
    Short = clangtypes.Short
    Int = clangtypes.Int
    Long = clangtypes.Long
    LongLong = clangtypes.LongLong
    Int128 = clangtypes.Int128
    Float = clangtypes.Float
    Double = clangtypes.Double
    LongDouble = clangtypes.LongDouble
    NullPtr = clangtypes.NullPtr
    ObjCId = clangtypes.ObjCId
    ObjCClass = clangtypes.ObjCClass
    ObjCSel = clangtypes.ObjCSel
    Dependent = clangtypes.Dependent
    Overload = clangtypes.Overload
    BoundMember = clangtypes.BoundMember
    UnknownAny = clangtypes.UnknownAny

    property kind:
        def __get__(self):
            return (<clangtypes.BuiltinType*>self.ptr).getKind()


cdef class FunctionType(Type):
    property result_type:
        def __get__(self):
            return create_QualType(
                (<clangtypes.FunctionType*>self.ptr).getResultType())


cdef class FunctionProtoType(FunctionType):
    pass


cdef class FunctionNoProtoType(FunctionType):
    pass


cdef create_Type(clangtypes.const_Type_ptr t):
    cdef Type type_ = {
        clangtypes.Builtin: BuiltinType,
        clangtypes.FunctionProto: FunctionProtoType,
        clangtypes.FunctionNoProto: FunctionNoProtoType
    }.get(t.getTypeClass(), Type)()
    type_.ptr = t
    return type_


cdef class QualType:
    cdef clangtypes.QualType *ptr
    def __dealloc__(self):
        if self.ptr:
            del self.ptr
    def __repr__(self):
        return "QualType(%r)" % self.type
    property type:
        def __get__(self):
            return create_Type(self.ptr.getTypePtr())


cdef create_QualType(clangtypes.QualType q):
    cdef QualType qual = QualType()
    qual.ptr = new clangtypes.QualType(q)
    return qual

###############################################################################


cdef class ParmVarDeclIterator:
    cdef decls.ParmVarDecl **begin
    cdef decls.ParmVarDecl **end
    def __next__(self):
        cdef decls.ParmVarDecl *decl
        if self.begin != self.end:
            decl = deref(self.begin)
            inc(self.begin)
            return create_Decl(decl)
        raise StopIteration()


cdef class FunctionParameterList:
    cdef decls.FunctionDecl *function
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


cdef class NamedDecl(Decl):
    property name:
        def __get__(self):
            cdef bytes name = \
                (<decls.ValueDecl*>self.ptr).getNameAsString().c_str()
            return name.decode()


cdef class ValueDecl(NamedDecl):
    property type:
        def __get__(self):
            return create_QualType((<decls.ValueDecl*>self.ptr).getType())


cdef class DeclaratorDecl(ValueDecl):
    pass


cdef class VarDecl(DeclaratorDecl):
    pass


cdef class ParmVarDecl(VarDecl):
    pass


cdef class FunctionDecl(DeclaratorDecl):
    property variadic:
        def __get__(self):
            return (<decls.FunctionDecl*>self.ptr).isVariadic()

    property parameters:
        def __get__(self):
            cdef decls.FunctionDecl *fd = <decls.FunctionDecl*>self.ptr
            cdef FunctionParameterList params = FunctionParameterList()
            params.function = <decls.FunctionDecl*>self.ptr
            return params


###############################################################################


cdef class DeclarationIterator:
    """
    Iterator for declarations in a DeclContext. There's no random access to
    declarations (that I can see?), so no __getattr__ is provided.
    """

    cdef readonly object translation_unit
    cdef decls.decl_iterator *begin
    cdef decls.decl_iterator *next_
    cdef decls.decl_iterator *end

    def __cinit__(self, translation_unit):
        self.translation_unit = translation_unit
        self.begin = NULL
        self.end = NULL

    def __dealloc__(self):
        if self.begin:
            del self.begin
        if self.end:
            del self.end

    def __iter__(self):
        return self

    def __next__(self):
        cdef decls.Decl *result
        if deref(self.next_) != deref(self.end):
            result = deref(deref(self.next_))
            inc(deref(self.next_))
            return create_Decl(result)
        raise StopIteration()


cdef class DeclContext:
    cdef readonly object translation_unit
    cdef decls.DeclContext *ptr
    def __cinit__(self, object translation_unit):
        self.translation_unit = translation_unit
        self.ptr = NULL

    property declarations:
        """
        Get an iterator to the declarations in this context.
        """
        def __get__(self):
            cdef DeclarationIterator iter_ = \
                DeclarationIterator(self.translation_unit)
            iter_.begin = new decls.decl_iterator(self.ptr.decls_begin())
            iter_.next_ = iter_.begin
            iter_.end = new decls.decl_iterator(self.ptr.decls_end())
            return iter_


###############################################################################


cdef class TranslationUnitDecl(Decl):
    cdef object parser

    def __init__(self, parser, capsule):
        assert PyCapsule_IsValid(capsule, <char*>0)
        cdef decls.TranslationUnitDecl *tu = \
            <decls.TranslationUnitDecl*>PyCapsule_GetPointer(capsule, <char*>0)
        self.parser = parser
        self.ptr = tu

    cdef DeclContext __getDeclContext(self):
        cdef DeclContext dc = DeclContext(self)
        dc.ptr = <decls.DeclContext*><decls.TranslationUnitDecl*>self.ptr
        return dc

    property declarations:
        def __get__(self):
            return self.decl_context.declarations

