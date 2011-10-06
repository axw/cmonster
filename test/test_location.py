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

class TestLocation(unittest.TestCase):
    def test_add(self):
        pp = cmonster.Preprocessor("test.c", data="ABC\n123")
        toks = [tok for tok in pp]
        self.assertEqual(2, len(toks))
        self.assertEqual("ABC", str(toks[0]))
        self.assertEqual("123", str(toks[1]))

        loc = toks[0].location
        self.assertEqual(1, loc.line)
        loc += len(toks[0]) + 1
        self.assertEqual(2, loc.line)

    def test_subtract(self):
        pp = cmonster.Preprocessor("test.c", data="ABC\n123")
        toks = [tok for tok in pp]
        self.assertEqual(2, len(toks))
        self.assertEqual("ABC", str(toks[0]))
        self.assertEqual("123", str(toks[1]))

        loc = toks[1].location
        self.assertEqual(2, loc.line)
        self.assertEqual(1, loc.column)
        loc -= 1
        self.assertEqual(1, loc.line)


if __name__ == "__main__":
    unittest.main()

