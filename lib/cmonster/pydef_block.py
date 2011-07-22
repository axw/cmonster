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

from ._preprocessor import *
from .python_block import PythonBlock


class PyDefBlock(PythonBlock):
    """
    PythonBlock subclass for handling "py_def" pragmas.
    """

    def __init__(self, preprocessor, signature):
        self.__preprocessor = preprocessor
        self.__signature = signature
        self.__tokens = ["def "] + list(signature) + [":\n"]

        # TODO verify signature is well formed.
        # Get the function name.
        self.__name = \
            (str(t) for t in signature if t.token_id==T_IDENTIFIER).__next__()


    def append(self, token):
        """
        Appends the provided token to the Python function body.
        """
        self.__tokens.append(token)


    def end(self):
        """
        Completes the py_def block. When this is called, the Python function
        macro will be defined.
        """
        # Compile and then define the function in the Python interpreter.
        code = compile("".join(str(t) for t in self.__tokens), "", "exec")
        locals_ = {}
        eval(code, globals(), locals_)
        function = locals_[self.__name]

        # Define the function macro in the preprocessor.
        self.__preprocessor.define(function)
        return None

