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

cdef class Type:
    cdef clang.types.const_Type_ptr ptr
    def __repr__(self):
        return "Type(%s)" % str(self)
    def __str__(self):
        return (<bytes>self.ptr.getTypeClassName()).decode()
    property builtin:
        def __get__(self): return self.ptr.isBuiltinType()


# Unfortunately, we can't directly access "cdef enum"'s from Python. So long,
# DRY.
class BuiltinTypeKind:
    def __init__(self, kind, name):
        self.kind = kind
        self.name = name
    def __repr__(self):
        return "BuiltinTypeKind(%d, %s)" % (self.kind, self.name)
BuiltinTypeKinds = {}
def add_type_kind(name, value):
    BuiltinTypeKinds[value] = BuiltinTypeKind(value, name)
add_type_kind("Void", clang.types.Void)
add_type_kind("Bool", clang.types.Bool)
add_type_kind("Char_U", clang.types.Char_U)
add_type_kind("UChar", clang.types.UChar)
add_type_kind("WChar_U", clang.types.WChar_U)
add_type_kind("Char16", clang.types.Char16)
add_type_kind("Char32", clang.types.Char32)
add_type_kind("UShort", clang.types.UShort)
add_type_kind("UInt", clang.types.UInt)
add_type_kind("ULong", clang.types.ULong)
add_type_kind("ULongLong", clang.types.ULongLong)
add_type_kind("UInt128", clang.types.UInt128)
add_type_kind("Char_S", clang.types.Char_S)
add_type_kind("SChar", clang.types.SChar)
add_type_kind("WChar_S", clang.types.WChar_S)
add_type_kind("Short", clang.types.Short)
add_type_kind("Int", clang.types.Int)
add_type_kind("Long", clang.types.Long)
add_type_kind("LongLong", clang.types.LongLong)
add_type_kind("Int128", clang.types.Int128)
add_type_kind("Float", clang.types.Float)
add_type_kind("Double", clang.types.Double)
add_type_kind("LongDouble", clang.types.LongDouble)
add_type_kind("NullPtr", clang.types.NullPtr)
add_type_kind("ObjCId", clang.types.ObjCId)
add_type_kind("ObjCClass", clang.types.ObjCClass)
add_type_kind("ObjCSel", clang.types.ObjCSel)
add_type_kind("Dependent", clang.types.Dependent)
add_type_kind("Overload", clang.types.Overload)
add_type_kind("BoundMember", clang.types.BoundMember)
add_type_kind("UnknownAny", clang.types.UnknownAny)

cdef class BuiltinType(Type):
    Void = BuiltinTypeKinds[clang.types.Void]
    Bool = BuiltinTypeKinds[clang.types.Bool]
    Char_U = BuiltinTypeKinds[clang.types.Char_U]
    UChar = BuiltinTypeKinds[clang.types.UChar]
    WChar_U = BuiltinTypeKinds[clang.types.WChar_U]
    Char16 = BuiltinTypeKinds[clang.types.Char16]
    Char32 = BuiltinTypeKinds[clang.types.Char32]
    UShort = BuiltinTypeKinds[clang.types.UShort]
    UInt = BuiltinTypeKinds[clang.types.UInt]
    ULong = BuiltinTypeKinds[clang.types.ULong]
    ULongLong = BuiltinTypeKinds[clang.types.ULongLong]
    UInt128 = BuiltinTypeKinds[clang.types.UInt128]
    Char_S = BuiltinTypeKinds[clang.types.Char_S]
    SChar = BuiltinTypeKinds[clang.types.SChar]
    WChar_S = BuiltinTypeKinds[clang.types.WChar_S]
    Short = BuiltinTypeKinds[clang.types.Short]
    Int = BuiltinTypeKinds[clang.types.Int]
    Long = BuiltinTypeKinds[clang.types.Long]
    LongLong = BuiltinTypeKinds[clang.types.LongLong]
    Int128 = BuiltinTypeKinds[clang.types.Int128]
    Float = BuiltinTypeKinds[clang.types.Float]
    Double = BuiltinTypeKinds[clang.types.Double]
    LongDouble = BuiltinTypeKinds[clang.types.LongDouble]
    NullPtr = BuiltinTypeKinds[clang.types.NullPtr]
    ObjCId = BuiltinTypeKinds[clang.types.ObjCId]
    ObjCClass = BuiltinTypeKinds[clang.types.ObjCClass]
    ObjCSel = BuiltinTypeKinds[clang.types.ObjCSel]
    Dependent = BuiltinTypeKinds[clang.types.Dependent]
    Overload = BuiltinTypeKinds[clang.types.Overload]
    BoundMember = BuiltinTypeKinds[clang.types.BoundMember]
    UnknownAny = BuiltinTypeKinds[clang.types.UnknownAny]
    
    property kind:
        def __get__(self):
            return BuiltinTypeKinds[
                (<clang.types.BuiltinType*>self.ptr).getKind()]
            return None

    def __repr__(self):
        return "BuiltinType(%r)" % self.kind


cdef class FunctionType(Type):
    property result_type:
        def __get__(self):
            return create_QualType(
                (<clang.types.FunctionType*>self.ptr).getResultType())


cdef class FunctionProtoType(FunctionType):
    pass


cdef class FunctionNoProtoType(FunctionType):
    pass


cdef create_Type(clang.types.const_Type_ptr t):
    cdef Type type_ = {
        clang.types.Builtin: BuiltinType,
        clang.types.FunctionProto: FunctionProtoType,
        clang.types.FunctionNoProto: FunctionNoProtoType
    }.get(t.getTypeClass(), Type)()
    type_.ptr = t
    return type_


cdef class QualType:
    cdef clang.types.QualType *ptr
    def __dealloc__(self):
        if self.ptr:
            del self.ptr
    def __repr__(self):
        return "QualType(%r)" % self.type
    property type:
        def __get__(self):
            return create_Type(self.ptr.getTypePtr())


cdef create_QualType(clang.types.QualType q):
    cdef QualType qual = QualType()
    qual.ptr = new clang.types.QualType(q)
    return qual

