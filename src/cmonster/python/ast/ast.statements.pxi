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


cdef class Statement:
    cdef clang.statements.Stmt *ptr
    def __repr__(self):
        return "Statement(%s)" % self.class_name
    property class_name:
        def __get__(self):
            return (<bytes>self.ptr.getStmtClassName()).decode()
    property children:
        def __get__(self):
            #cdef StatementRange range_ = StatementRange()
            #range_.ptr = new clang.statements.StmtRange(self.ptr.children())
            cdef clang.statements.StmtRange *range_ = \
                new clang.statements.StmtRange(self.ptr.children())
            try:
                while <bint>deref(range_):
                    yield create_Statement(deref(deref(range_)))
                    inc(deref(range_))
            finally:
                del range_


cdef class StatementRange:
    cdef clang.statements.StmtRange *ptr
    def __dealloc__(self):
        if self.ptr: del self.ptr
    def __nonzero__(self):
        return self.ptr != NULL and <bint>deref(self.ptr)


cdef class StatementIterator:
    cdef clang.statements.Stmt **begin
    cdef clang.statements.Stmt **end
    def __next__(self):
        cdef clang.statements.Stmt *result
        if self.begin != self.end:
            result = deref(self.begin)
            inc(self.begin)
            return create_Statement(result)
        raise StopIteration()


cdef class StatementList:
    cdef clang.statements.Stmt **begin
    cdef clang.statements.Stmt **end
    def __len__(self):
        return <long>(self.end-self.begin)
    def __iter__(self):
        cdef StatementIterator iter_ = StatementIterator()
        iter_.begin = self.begin
        iter_.end = self.end
        return iter_
    def __repr__(self):
        return repr([s for s in self])
    def __getitem__(self, i):
        if i < 0 or i >= len(self):
            raise IndexError("Index out of range")
        return create_Statement(self.begin[i])


###############################################################################


cdef class CompoundStatement(Statement):
    property body:
        def __get__(self):
            cdef clang.statements.CompoundStmt *this = \
                <clang.statements.CompoundStmt*>self.ptr
            cdef StatementList list_ = StatementList()
            list_.begin = this.body_begin()
            list_.end = this.body_end()
            return list_


cdef class ReturnStatement(Statement):
    property return_value:
        def __get__(self):
            cdef clang.statements.ReturnStmt *rs = \
                <clang.statements.ReturnStmt*>self.ptr
            cdef clang.exprs.Expr *expr = rs.getRetValue()
            if expr != NULL:
                return create_Statement(expr)


###############################################################################


cdef Statement create_Statement(clang.statements.Stmt *ptr):
    cdef Statement stmt = {
        clang.statements.CompoundStmtClass: CompoundStatement,
        clang.statements.ReturnStmtClass: ReturnStatement,
        clang.statements.ImplicitCastExprClass: ImplicitCastExpr,
        clang.statements.UnaryOperatorClass: UnaryOperator,
        clang.statements.IntegerLiteralClass: IntegerLiteral,
        clang.statements.DeclRefExprClass: DeclRefExpr
    }.get(ptr.getStmtClass(), Statement)()
    stmt.ptr = ptr
    return stmt

