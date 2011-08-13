# Copyright (c) 2011 Andrew Wilkins <axwalk@gmail.com>
# 
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

import cmonster
import os
import unittest

class TestDefine(unittest.TestCase):
    def test_define_object(self):
        pp = cmonster.Preprocessor("test.c", data="ABC")
        pp.define("ABC", "123")
        toks = [tok for tok in pp]
        self.assertEqual(1, len(toks))
        self.assertEqual("123", str(toks[0]))


    def test_define_simple_function(self):
        pp = cmonster.Preprocessor("test.c", data="ABC(123)")
        pp.define("ABC(X)", "X")
        toks = [tok for tok in pp]
        self.assertEqual(1, len(toks))
        self.assertEqual("123", str(toks[0]))


    def test_define_python_function(self):
        def ABC(arg):
            return "".join(reversed(str(arg)))
        pp = cmonster.Preprocessor("test.c", data="ABC(321)")
        pp.define(ABC)
        toks = [tok for tok in pp]
        self.assertEqual(1, len(toks))
        self.assertEqual("123", str(toks[0]))


    def test_define_python_function_with_name(self):
        def XYZ(x):
            return "123"
        pp = cmonster.Preprocessor("test.c", data="ABC(123)")
        pp.define("ABC", XYZ)
        toks = [tok for tok in pp]
        self.assertEqual(1, len(toks))
        self.assertEqual("123", str(toks[0]))


if __name__ == "__main__":
    unittest.main()

