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

import os
import subprocess


def _get_predefined_macros(executable):
    "Determine the predefined macros for gcc/g++."
    proc = subprocess.Popen(
        [executable, "-E", "-dM", "-"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if proc.returncode != 0:
        msg = "Failed to determine predefined macros:\n%s" % stderr
        raise RuntimeError(msg)
    for line in stdout.splitlines():
        line = line.decode()[len("#define "):].rstrip()
        space = line.find(" ")
        macro = (line[:space], line[space+1:])
        yield macro


class IncludeLocator:
    """
    An "include locator" that consults GCC for the location of an include
    file.
    
    On success, the parent directory will be added to the preprocessor as a
    system include directory.
    """

    def __init__(self, preprocessor, executable):
        self.__preprocessor = preprocessor
        self.__executable = executable

    def __call__(self, include):
        # TODO detect language, stop assuming C++.
        proc = subprocess.Popen([self.__executable, "-x", "c++", "-E", "-"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                stdin=subprocess.PIPE)
        stdout, stderr = proc.communicate(("#include %s" % include).encode())
        # We ignore the return code, since an error might be due to an
        # error directive in the header, not just due to the preprocessor not
        # having found the include.
        for line in stdout.splitlines():
            # The first line starting with '#' and not containing '<' is
            # the line marker we're after. The preceding line-markers are
            # for meta files (<stdin>, <command-line>, etc.)
            if line.startswith(b"#") and line.find(b"<") == -1:
                start = line.find(b'"')
                assert start != -1
                end = line.rfind(b'"')
                assert end > start
                path = line[start+1:end].decode()

                # Add the parent directory to the preprocessor's
                # system include paths.
                norm_include = os.path.normpath(include[1:-1])
                abs_path = os.path.abspath(path)
                assert abs_path.endswith(norm_include)
                include_dir = abs_path[:-len(norm_include)-1]
                self.__preprocessor.add_include_dir(include_dir, True)

                # Return the 
                return abs_path


def configure(preprocessor, executable="g++"):
    """
    Add the gcc/g++ predefined macros and system include paths to the cmonster
    preprocessor object.
    """

    # Define builtin macros.
    for (name, value) in _get_predefined_macros(executable):
        preprocessor.define(name, value)

    # Add an include locator.
    preprocessor.set_include_locator(IncludeLocator(preprocessor, executable))

