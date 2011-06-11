from distutils.core import setup
from distutils.extension import Extension

# Remove the "-Wstrict-prototypes" compiler option, which isn't valid for C++.
import distutils.sysconfig
cfg_vars = distutils.sysconfig.get_config_vars()
if "CFLAGS" in cfg_vars:
    cfg_vars["CFLAGS"] = cfg_vars["CFLAGS"].replace("-Wstrict-prototypes", "")

# "_preprocessor" extension module.
_preprocessor_extension = Extension(
    "csnake._preprocessor",
    [
        "src/csnake/core/function_macro.cpp",
        "src/csnake/core/preprocessor.cpp",
        "src/csnake/core/token_iterator.cpp",
        "src/csnake/python/function_macro.cpp",
        "src/csnake/python/module.cpp",
        "src/csnake/python/preprocessor.cpp",
        "src/csnake/python/token.cpp",
        "src/csnake/python/token_iterator.cpp",
    ],
    libraries = ["boost_wave"] # XXX Le sigh. I can't specify "-static"?
)

setup(
    name="csnake",
    version="0.1",
    ext_modules=[_preprocessor_extension]
)

