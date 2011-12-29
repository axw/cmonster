from distribute_setup import use_setuptools
use_setuptools()

from setuptools import setup
from setuptools.extension import Extension
import Cython.Distutils

description = """
cmonster is a Python wrapper around the Clang/LLVM preprocessor, adding support
for inline Python macros, programmatic #include handling, and external
preprocessor emulation.
""".strip()

# Remove the "-Wstrict-prototypes" compiler option, which isn't valid for C++.
import distutils.sysconfig
cfg_vars = distutils.sysconfig.get_config_vars()
if "CFLAGS" in cfg_vars:
    cfg_vars["CFLAGS"] = cfg_vars["CFLAGS"].replace("-Wstrict-prototypes", "")


# We embed a second extension module in one shared library, so we need to
# wrangle Cython a bit here to specify the correct extension name.
class build_ext(Cython.Distutils.build_ext):
    def __init__(self, *args, **kwargs):
        Cython.Distutils.build_ext.__init__(self, *args, **kwargs)
    def cython_sources(self, sources, extension):
        old_name = extension.name
        extension.name = "cmonster._cmonster_ast"
        try:
            return Cython.Distutils.build_ext.cython_sources(
                self, sources, extension)
        finally:
            extension.name = old_name


# "cmonster-core" shared library.
_cmonster_extension = Extension(
    "cmonster._cmonster",
    [
        "src/cmonster/core/impl/exception_diagnostic_client.cpp",
        "src/cmonster/core/impl/include_locator_impl.cpp",
        "src/cmonster/core/impl/function_macro.cpp",
        "src/cmonster/core/impl/parser.cpp",
        "src/cmonster/core/impl/parse_result.cpp",
        "src/cmonster/core/impl/preprocessor_impl.cpp",
        "src/cmonster/core/impl/token_iterator.cpp",
        "src/cmonster/core/impl/token_predicate.cpp",
        "src/cmonster/core/impl/token.cpp",

        "src/cmonster/python/exception.cpp",
        "src/cmonster/python/include_locator.cpp",
        "src/cmonster/python/function_macro.cpp",
        "src/cmonster/python/module.cpp",
        "src/cmonster/python/parser.cpp",
        "src/cmonster/python/parse_result.cpp",
        "src/cmonster/python/preprocessor.cpp",
        "src/cmonster/python/pyfile_ostream.cpp",
        "src/cmonster/python/rewriter.cpp",
        "src/cmonster/python/source_location.cpp",
        "src/cmonster/python/token.cpp",
        "src/cmonster/python/token_iterator.cpp",
        "src/cmonster/python/token_predicate.cpp",

        "src/cmonster/python/ast/ast.pyx",

        "src/cmonster/python/ast/clang.astcontext.pxd",
        "src/cmonster/python/ast/clang.decls.pxd",
        "src/cmonster/python/ast/clang.exprs.pxd",
        "src/cmonster/python/ast/clang.source.pxd",
        "src/cmonster/python/ast/clang.statements.pxd",
        "src/cmonster/python/ast/clang.types.pxd",
        "src/cmonster/python/ast/llvm.pxd",

        "src/cmonster/python/ast/ast.astcontext.pxi",
        "src/cmonster/python/ast/ast.declcontext.pxi",
        "src/cmonster/python/ast/ast.decls.pxi",
        "src/cmonster/python/ast/ast.exprs.pxi",
        "src/cmonster/python/ast/ast.source.pxi",
        "src/cmonster/python/ast/ast.statements.pxi",
        "src/cmonster/python/ast/ast.types.pxi"
    ],

    # Tell Cython to compile in C++ mode.
    language = "c++",

    # Required by LLVM/Clang.
    define_macros = [("__STDC_LIMIT_MACROS", 1),
                     ("__STDC_CONSTANT_MACROS", 1)],

    # LLVM/Clang include directories.
    include_dirs = [
        "src",
        "clang/include"
    ],

    # LLVM/Clang libraries.
    library_dirs = ["clang/lib"],
    libraries = [
        "clangRewrite",
        "clangFrontend",
        "clangDriver",
        "clangSerialization",
        "clangParse",
        "clangSema",
        "clangAnalysis",
        "clangAST",
        "clangLex",
        "clangBasic",
        "LLVMMC",
        "LLVMSupport",
        "LLVMCore",
        "pthread",
        "dl"
    ],

    # No RTTI in Clang, so none here either.
    extra_compile_args = ["-fno-rtti"]
)


classifiers = [
    "Intended Audience :: Developers",
    "Operating System :: POSIX",
    "Programming Language :: Python :: 3.2",
    "Programming Language :: C",
    "Programming Language :: C++",
    "License :: OSI Approved :: MIT License",
    "Development Status :: 3 - Alpha",
    "Topic :: Software Development :: Pre-processors"
]


setup(
    name="cmonster",
    version="0.2",
    classifiers=classifiers,
    description=description,
    packages = ["cmonster", "cmonster.config"],
    package_dir = {"": "lib"},
    scripts = ["scripts/cmonster"],
    ext_modules=[_cmonster_extension],
    author="Andrew Wilkins",
    author_email="axwalk@gmail.com",
    url="http://github.com/axw/cmonster",
    test_suite="test",
    cmdclass = {"build_ext": build_ext},
)

