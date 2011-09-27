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

cdef class DeclarationIterator:
    """
    Iterator for declarations in a DeclContext. There's no random access to
    declarations (that I can see?), so no __getattr__ is provided.
    """

    cdef readonly object translation_unit
    cdef clang.decls.decl_iterator *begin
    cdef clang.decls.decl_iterator *next_
    cdef clang.decls.decl_iterator *end

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
        cdef clang.decls.Decl *result
        if deref(self.next_) != deref(self.end):
            result = deref(deref(self.next_))
            inc(deref(self.next_))
            return create_Decl(result)
        raise StopIteration()


cdef class DeclContext:
    cdef readonly object translation_unit
    cdef clang.decls.DeclContext *ptr
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
            iter_.begin = new clang.decls.decl_iterator(self.ptr.decls_begin())
            iter_.next_ = iter_.begin
            iter_.end = new clang.decls.decl_iterator(self.ptr.decls_end())
            return iter_

