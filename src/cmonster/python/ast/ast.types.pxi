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

