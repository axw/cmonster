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


cdef class Expr(Statement):
    property type:
        def __get__(self):
            return create_QualType((<clang.exprs.Expr*>self.ptr).getType())


cdef class CastExpr(Expr):
    property subexpr:
        def __get__(self):
            cdef clang.exprs.CastExpr *this = <clang.exprs.CastExpr*>self.ptr
            cdef clang.exprs.Expr *expr = this.getSubExpr()
            if expr != NULL:
                return create_Statement(expr)


cdef class ImplicitCastExpr(CastExpr):
    pass

###############################################################################

cdef class IntegerLiteral(Expr):
    property value:
        def __get__(self):
            cdef clang.exprs.IntegerLiteral*this = \
                <clang.exprs.IntegerLiteral*>self.ptr
            cdef llvm.APInt apint = this.getValue()
            cdef unsigned nwords = apint.getNumWords()
            cdef llvm.const_uint64_t_ptr words = apint.getRawData()
            # TODO Verify that words are stored in MSB order.
            value = 0
            for i from 0 <= i < nwords:
                value = (value << 64) | words[i]
            return value

###############################################################################

class UnaryOperatorKind:
    def __init__(self, kind, name):
        self.kind = kind
        self.name = name
    def __repr__(self):
        return "UnaryOperatorKind(%d, %s)" % (self.kind, self.name)
UnaryOperatorKinds = {}
def add_unaryoperator_kind(name, value):
    UnaryOperatorKinds[value] = UnaryOperatorKind(value, name)
add_unaryoperator_kind("PostInc", clang.exprs.UO_PostInc)
add_unaryoperator_kind("PostDec", clang.exprs.UO_PostDec)
add_unaryoperator_kind("PreInc", clang.exprs.UO_PreInc)
add_unaryoperator_kind("PreDec", clang.exprs.UO_PreDec)
add_unaryoperator_kind("AddrOf", clang.exprs.UO_AddrOf)
add_unaryoperator_kind("Deref", clang.exprs.UO_Deref)
add_unaryoperator_kind("Plus", clang.exprs.UO_Plus)
add_unaryoperator_kind("Minus", clang.exprs.UO_Minus)
add_unaryoperator_kind("Not", clang.exprs.UO_Not)
add_unaryoperator_kind("LNot", clang.exprs.UO_LNot)
add_unaryoperator_kind("Real", clang.exprs.UO_Real)
add_unaryoperator_kind("Imag", clang.exprs.UO_Imag)
add_unaryoperator_kind("Extension", clang.exprs.UO_Extension)


cdef class UnaryOperator(Expr):
    PostInc = UnaryOperatorKinds[clang.exprs.UO_PostInc]
    PostDec = UnaryOperatorKinds[clang.exprs.UO_PostDec]
    PreInc = UnaryOperatorKinds[clang.exprs.UO_PreInc]
    PreDec = UnaryOperatorKinds[clang.exprs.UO_PreDec]
    AddrOf = UnaryOperatorKinds[clang.exprs.UO_AddrOf]
    Deref = UnaryOperatorKinds[clang.exprs.UO_Deref]
    Plus = UnaryOperatorKinds[clang.exprs.UO_Plus]
    Minus = UnaryOperatorKinds[clang.exprs.UO_Minus]
    Not = UnaryOperatorKinds[clang.exprs.UO_Not]
    LNot = UnaryOperatorKinds[clang.exprs.UO_LNot]
    Real = UnaryOperatorKinds[clang.exprs.UO_Real]
    Imag = UnaryOperatorKinds[clang.exprs.UO_Imag]
    Extension = UnaryOperatorKinds[clang.exprs.UO_Extension]

    property opcode:
        def __get__(self):
            cdef clang.exprs.UnaryOperator *this = \
                <clang.exprs.UnaryOperator*>self.ptr
            return UnaryOperatorKinds[this.getOpcode()]

    property subexpr:
        def __get__(self):
            cdef clang.exprs.UnaryOperator *this = \
                <clang.exprs.UnaryOperator*>self.ptr
            cdef clang.exprs.Expr *expr = this.getSubExpr()
            if expr != NULL:
                return create_Statement(expr)

###############################################################################

cdef class DeclRefExpr(Expr):
    property decl:
        def __get__(self):
            cdef clang.exprs.DeclRefExpr *this = \
                <clang.exprs.DeclRefExpr*>self.ptr
            return create_Decl(this.getDecl())

