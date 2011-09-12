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

from . import _cmonster
from . import _preprocessor
from . import config

class Parser(_cmonster.Parser):
    def __init__(self, filename, data=None):
        if data is None:
            if type(filename) is str:
                data = open(filename).read()
            else:
                # Assume 'filename' is a file.
                data = filename.read()
                if hasattr(filename, "name"):
                    filename = filename.name
        _cmonster.Parser.__init__(self, data, filename)

        # TODO allow configuration of target preprocessor/compiler.
        pp = self.preprocessor
        config.configure(pp)

        # XXX Should this be configurable?
        pp.add_include_dir(".", False)

        # Predefined macros.
        pp.define('py_def', _preprocessor.PyDefHandler(pp))

