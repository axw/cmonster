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

from cpython.pycapsule cimport \
    PyCapsule_New, PyCapsule_IsValid, PyCapsule_GetPointer
from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc
from libc.stdint cimport uint64_t

cimport llvm
cimport clang.astcontext
cimport clang.exprs
cimport clang.types
cimport clang.decls
cimport clang.source
cimport clang.statements

include "ast.astcontext.pxi"
include "ast.source.pxi"
include "ast.types.pxi"
include "ast.decls.pxi"
include "ast.declcontext.pxi"
include "ast.statements.pxi"
include "ast.exprs.pxi"

