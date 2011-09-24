cdef extern from "clang/AST/Type.h" namespace "clang::Type":
    cdef enum TypeClass:
        pass

cdef extern from "clang/AST/Type.h" namespace "clang":
    cdef cppclass Type:
        TypeClass getTypeClass()

    cdef cppclass QualType:
        Type& operator*()
        bint isConstQualified()
        bint isVolatileQualified()

