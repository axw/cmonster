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

cdef extern from "clang/AST/Type.h" namespace "clang::Type":
    cdef enum TypeClass:
        Builtin
        FunctionProto
        FunctionNoProto

cdef extern from "clang/AST/Type.h" namespace "clang::BuiltinType":
    cdef enum Kind:
        Void
        Bool
        Char_U
        UChar
        WChar_U
        Char16
        Char32
        UShort
        UInt
        ULong
        ULongLong
        UInt128
        Char_S
        SChar
        WChar_S
        Short
        Int
        Long
        LongLong
        Int128
        Float
        Double
        LongDouble
        NullPtr
        ObjCId
        ObjCClass
        ObjCSel
        Dependent
        Overload
        BoundMember
        UnknownAny

cdef extern from "clang/AST/Type.h" namespace "clang":
    cdef cppclass Type:
        TypeClass getTypeClass()
        char *getTypeClassName()
        bint isBuiltinType()
    ctypedef Type* const_Type_ptr "const clang::Type*"


    cdef cppclass BuiltinType(Type):
        Kind getKind()


    cdef cppclass FunctionType(Type):
        QualType getResultType()


    cdef cppclass QualType:
        QualType(QualType&)
        Type* getTypePtr()
        bint isConstQualified()
        bint isVolatileQualified()

