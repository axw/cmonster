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

cdef class SourceLocation:
    cdef clang.source.SourceLocation *ptr
    cdef clang.source.SourceManager *mgr
    def __dealloc__(self):
        if self.ptr: del self.ptr
    def __int__(self):
        return <int>self.ptr.getRawEncoding()
    property valid:
        def __get__(self): return self.ptr.isValid()
    property invalid:
        def __get__(self): return self.ptr.isInvalid()


cdef class FileSourceLocation(SourceLocation):
    def __repr__(self):
        # TODO just use the builtin "print" method of SourceLocation?
        cdef clang.source.PresumedLoc ploc
        if self.valid and self.mgr != NULL:
            ploc = self.mgr.getPresumedLoc(deref(self.ptr))
            if ploc.isInvalid():
                return "SourceLocation(<Invalid>)"
            return "SourceLocation(%s:%d:%d)" % \
                ((<bytes>ploc.getFilename()).decode(),
                 ploc.getLine(), ploc.getColumn())
        else:
            return "SourceLocation(<Invalid>)"

    property filename:
        def __get__(self):
            ploc = self.mgr.getPresumedLoc(deref(self.ptr))
            assert ploc.isValid()
            return (<bytes>ploc.getFilename()).decode()

    property line:
        def __get__(self):
            ploc = self.mgr.getPresumedLoc(deref(self.ptr))
            assert ploc.isValid()
            return ploc.getLine()

    property column:
        def __get__(self):
            ploc = self.mgr.getPresumedLoc(deref(self.ptr))
            assert ploc.isValid()
            return ploc.getColumn()


cdef class MacroSourceLocation(SourceLocation):
    def __repr__(self):
        # TODO just use the builtin "print" method of SourceLocation?
        if self.isvalid:
            return "SourceLocation(<Invalid>)"
        return "SourceLocation()"


cdef SourceLocation \
    create_SourceLocation(clang.source.SourceLocation loc,
                          clang.source.SourceManager *mgr = NULL):
    cdef SourceLocation sl
    if loc.isFileID():
        sl = FileSourceLocation()
    else:
        sl = MacroSourceLocation()
    sl.ptr = new clang.source.SourceLocation(loc)
    sl.mgr = mgr
    return sl

