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

cdef extern from "clang/Basic/SourceLocation.h" namespace "clang":
    cdef cppclass SourceLocation:
        SourceLocation(SourceLocation&)
        bint isFileID()
        bint isMacroID()
        bint isValid()
        bint isInvalid()
        unsigned getRawEncoding()

    cdef cppclass PresumedLoc:
        bint isValid()
        bint isInvalid()
        char* getFilename()
        unsigned getLine()
        unsigned getColumn()
        SourceLocation getIncludeLoc()

cdef extern from "clang/Basic/SourceManager.h" namespace "clang":
    cdef cppclass SourceManager:
        PresumedLoc getPresumedLoc(SourceLocation)

