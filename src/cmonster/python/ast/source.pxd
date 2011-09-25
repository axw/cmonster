cdef extern from "clang/Basic/SourceLocation.h" namespace "clang":
    cdef cppclass SourceLocation:
        SourceLocation(SourceLocation&)
        bint isFileID()
        bint isMacroID()
        bint isValid()
        bint isInvalid()

