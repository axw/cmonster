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
    "Preprocessor", "Token",
] + [name for name in locals() if name.startswith("T_")]


# Import Python block classes.
from .pydef_block import PyDefBlock


def _CSNAKE_STR(*tokens):
    """
    Built-in macro to create a single string literal token from the
    concatenation of each of the token values.
    """
    if tokens:
        return [Token(T_STRINGLIT, '"%s"' % "".join(str(t) for t in tokens))]


class TokenIteratorWrapper(object):
    """
    A class to wrap the native _preprocessor.TokenIterator class. This class
    provides additional support for the built-in Python block support pragmas.
    """

    def __init__(self, preprocessor, iterator):
        self.__preprocessor = preprocessor
        self.__iterator = iterator
        self.__pyblocks = []


    def __iter__(self):
        return self


    def __next__(self):
        if self.__iterator:
            try:
                return self._get_token()
            except:
                self.__iterator = None
                raise
        raise StopIteration


    def _get_token(self):
        token = self.__iterator.__next__()
        while self.__pyblocks:
            self.__pyblocks[-1].append(token)
            token = self.__iterator.__next__()
        return token


    def _pydef(self, args):
        assert not self.__pyblocks, \
            "pydef macros may not be defined within another Python block"
        block = PyDefBlock(self.__preprocessor, args)
        self.__pyblocks.append(block)


    def _pyend(self, args):
        block = self.__pyblocks.pop()
        # TODO handle nested blocks
        return block.end()


_Preprocessor = Preprocessor # _preprocessor.Preprocessor
class Preprocessor(_Preprocessor):
    """
    Main C Preprocessor class.
    """

    def __init__(self, filename):
        _Preprocessor.__init__(self, filename)
        self.__iterator = None

        # Predefined custom pragmas.
        self.add_pragma("py_def", self.__pydef)
        self.add_pragma("py_end", self.__pyend)

        # Predefined macros.
        #
        # _CSNAKE_STR
        self.define(_CSNAKE_STR)
        # py_def
        self.define('py_def(fn)=_Pragma(_CSNAKE_STR(wave py_def(fn)))')
        # py_end
        self.define('py_end=_Pragma("wave py_end")')


    def preprocess(self):
        """
        Main entrypoint method to the preprocessor. Returns an token iterator.
        """
        assert self.__iterator is None
        self.__iterator = TokenIteratorWrapper(
            self, _Preprocessor.preprocess(self))
        return self.__iterator


    def __pydef(self, *args):
        """
        Callback method for handling "py_def" pragmas.
        """
        return self.__iterator._pydef(args)


    def __pyend(self, *args):
        """
        Callback method for handling "py_end" pragmas.
        """
        return self.__iterator._pyend(args)

