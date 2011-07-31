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

import sys
if sys.version_info < (3, 2):
    print >> sys.stderr, "Python version 3.2+ is required"
    sys.exit(1)

# Import the extension module's contents, so we get all of the token IDs.
from ._preprocessor import *


# Define the names to import from this module.
__all__ = [
    "Preprocessor", "Token"
] + [name for name in locals() if name.startswith("tok_")]


_Preprocessor = Preprocessor # _preprocessor.Preprocessor
class Preprocessor(_Preprocessor):
    """
    Main C Preprocessor class.
    """

    def __init__(self, filename, include_paths=()):
        _Preprocessor.__init__(self, filename, include_paths)

        # Predefined macros.
        #
        # py_def
        self.define('py_def', self.__pydef)


    def preprocess(self, f=None):
        """
        Preprocesses the source and prints the output to the specified file.
        """
        if f is None:
            _Preprocessor.preprocess(self, sys.stdout.fileno())
        elif type(f) is str:
            with open(f) as f_:
                _Preprocessor.preprocess(self, f_.fileno())
        else:
            # Assume it's a file-like object with a fileno() method.
            return _Preprocessor.preprocess(self, f.fileno())


    def __pydef(self, *signature_tokens):
        """
        Callback method for handling "py_def" pragmas.
        """

        # Grab all of the tokens up to and including the "py_end" token.
        body = []
        while True:
            tok = self.next(False) # Get next unexpanded token
            if tok.token_id == tok_identifier and str(tok) == "py_end":
                break
            body.append(tok)

        # Format the Python function.
        signature = self.format_tokens(signature_tokens)
        body = self.format_tokens(body)
        function_source = "def %s:\n%s" % (signature, body)

        # Compile the Python function.
        code = compile(function_source, "<cmonster>", "exec")
        locals_ = {}
        eval(code, globals(), locals_)
        fn = locals_[str(signature_tokens[0])]
        self.define(fn)

