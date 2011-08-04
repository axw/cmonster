from distutils.core import setup
from distutils.extension import Extension

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

# "_preprocessor" extension module.
_preprocessor_extension = Extension(
    "cmonster._preprocessor",
    [
        "src/cmonster/core/impl/include_locator_impl.cpp",
        "src/cmonster/core/impl/function_macro.cpp",
        "src/cmonster/core/impl/preprocessor.cpp",
        "src/cmonster/core/impl/token_iterator.cpp",
        "src/cmonster/core/impl/token_predicate.cpp",
        "src/cmonster/core/impl/token.cpp",
        "src/cmonster/python/include_locator.cpp",
        "src/cmonster/python/function_macro.cpp",
        "src/cmonster/python/module.cpp",
        "src/cmonster/python/preprocessor.cpp",
        "src/cmonster/python/token.cpp",
        "src/cmonster/python/token_iterator.cpp",
        "src/cmonster/python/token_predicate.cpp"
    ],

    # Required by LLVM/Clang.
    define_macros = [("__STDC_LIMIT_MACROS", 1),
                     ("__STDC_CONSTANT_MACROS", 1)],

    # LLVM/Clang include directories.
    include_dirs = [
        "src",
        "/home/andrew/prog/llvm/tools/clang/include/",
        "/home/andrew/prog/llvm/build/tools/clang/include/",
        "/home/andrew/prog/llvm/include/",
        "/home/andrew/prog/llvm/build/include/"
    ],

    # LLVM/Clang libraries.
    #library_dirs = ["/home/andrew/prog/llvm/build/Debug+Asserts/lib"],
    library_dirs = ["/home/andrew/prog/llvm/build/Release/lib"],
    libraries = [
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

classifiers =[
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
    version="0.1",
    classifiers=classifiers,
    description=description,
    packages = ["cmonster", "cmonster.config"],
    package_dir = {"": "lib"},
    scripts = ["scripts/cmonster"],
    ext_modules=[_preprocessor_extension],
    author="Andrew Wilkins",
    author_email="axwalk@gmail.com",
    url="http://github.com/axw/cmonster"
)

