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

from clang.exprs cimport Expr


cdef extern from "clang/AST/Stmt.h" namespace "clang::Stmt":
    cdef enum StmtClass:
        CompoundStmtClass
        ReturnStmtClass
        ImplicitCastExprClass
        IntegerLiteralClass


cdef extern from "clang/AST/StmtIterator.h" namespace "clang":
    cdef cppclass StmtRange:
        StmtRange(StmtRange&)
        StmtRange& operator++()
        Stmt* operator*()


cdef extern from "clang/AST/Stmt.h" namespace "clang":
    cdef cppclass Stmt:
        StmtClass getStmtClass()
        char* getStmtClassName()
        StmtRange children()

    cdef cppclass CompoundStmt(Stmt):
        unsigned size()
        Stmt** body_begin()
        Stmt** body_end()

    cdef cppclass ReturnStmt(Stmt):
        Expr* getRetValue()

