from cpython.pycapsule cimport PyCapsule_IsValid, PyCapsule_GetPointer
from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc

cimport decls

cdef class Decl:
    cdef decls.Decl *ptr

    def __str__(self):
        return self.kind_name

    cdef DeclContext __getDeclContext(self):
        cdef DeclContext dc = DeclContext.__new__(DeclContext)
        dc.ptr = self.ptr.getDeclContext()
        assert dc.ptr != NULL
        return dc

    def __repr__(self):
        return "Decl(%s)" % self.kind_name

    property kind:
        def __get__(self):
            return self.ptr.getKind()

    property kind_name:
        def __get__(self):
            return (<bytes>self.ptr.getDeclKindName()).decode("UTF-8")

    property body:
        def __get__(self):
            # TODO
            print(self.ptr.hasBody())

    property decl_context:
        def __get__(self):
            return self.__getDeclContext()


cdef Decl create_Decl(decls.Decl *d):
    cdef Decl decl
    decl_type = {
        decls.Function: FunctionDecl,
    }.get(d.getKind(), Decl)
    decl = decl_type.__new__(decl_type)
    decl.ptr = d
    return decl


###############################################################################


cdef class ParmVarDeclIterator:
    cdef decls.ParmVarDecl **begin
    cdef decls.ParmVarDecl **end
    def __next__(self):
        cdef decls.ParmVarDecl *decl
        if self.begin != self.end:
            decl = deref(self.begin)
            inc(self.begin)
            return create_Decl(decl)
        raise StopIteration()


cdef class FunctionParameterList:
    cdef decls.FunctionDecl *function
    def __len__(self):
        return self.function.param_size()
    def __iter__(self):
        cdef ParmVarDeclIterator iter_ = ParmVarDeclIterator()
        iter_.begin = self.function.param_begin()
        iter_.end = self.function.param_end()
        return iter_
    def __repr__(self):
        return repr([p for p in self])
    def __getitem__(self, i):
        if i < 0 or i >= len(self):
            raise IndexError("Parameter index out of range")


cdef class NamedDecl(Decl):
    pass

cdef class ValueDecl(NamedDecl):
    pass

cdef class DeclaratorDecl(ValueDecl):
    pass

cdef class FunctionDecl(DeclaratorDecl):
    property variadic:
        def __get__(self):
            return (<decls.FunctionDecl*>self.ptr).isVariadic()

    property parameters:
        def __get__(self):
            cdef decls.FunctionDecl *fd = <decls.FunctionDecl*>self.ptr
            cdef FunctionParameterList params = FunctionParameterList()
            params.function = <decls.FunctionDecl*>self.ptr
            return params


###############################################################################


cdef class DeclarationIterator:
    cdef readonly object translation_unit
    cdef decls.decl_iterator *begin
    cdef decls.decl_iterator *next_
    cdef decls.decl_iterator *end

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
        cdef decls.Decl *result
        if deref(self.next_) != deref(self.end):
            result = deref(deref(self.next_))
            inc(deref(self.next_))
            return create_Decl(result)
        raise StopIteration()


cdef class DeclContext:
    cdef readonly object translation_unit
    cdef decls.DeclContext *ptr

    def __cinit__(self, object translation_unit):
        self.translation_unit = translation_unit
        self.ptr = NULL

    property declarations:
        def __get__(self):
            cdef DeclarationIterator iter_ = \
                DeclarationIterator(self.translation_unit)
            iter_.begin = new decls.decl_iterator(self.ptr.decls_begin())
            iter_.next_ = iter_.begin
            iter_.end = new decls.decl_iterator(self.ptr.decls_end())
            return iter_


###############################################################################


cdef class TranslationUnitDecl(Decl):
    cdef object parser

    def __init__(self, parser, capsule):
        assert PyCapsule_IsValid(capsule, <char*>0)
        cdef decls.TranslationUnitDecl *tu = \
            <decls.TranslationUnitDecl*>PyCapsule_GetPointer(capsule, <char*>0)
        self.parser = parser
        self.ptr = tu

    cdef DeclContext __getDeclContext(self):
        cdef DeclContext dc = DeclContext(self)
        dc.ptr = <decls.DeclContext*><decls.TranslationUnitDecl*>self.ptr
        return dc

    property declarations:
        def __get__(self):
            return self.decl_context.declarations

