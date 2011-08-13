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
import tempfile
import unittest

def get_new_filename():
    "Return a filename which does not exist in the current working directory."
    i = 0
    while os.path.exists(str(i)):
        i += 1
    return str(i)


class TestIncludeLocator(unittest.TestCase):
    def test_include_filename(self):
        with tempfile.TemporaryDirectory() as d:
            with open(d + "/" + get_new_filename(), "w") as f:
                f.write("abc\n")

            pp = cmonster.Preprocessor(
                "test.c",
                data="#include \"%s\"" % os.path.basename(f.name))
            pp.add_include_dir(os.path.dirname(f.name))

            tokens = [t for t in pp]
            self.assertEqual(1, len(tokens))
            self.assertEqual(cmonster.tok_identifier, tokens[0].token_id)
            self.assertEqual("abc", str(tokens[0]))


    def test_include_cwd(self):
        with tempfile.NamedTemporaryFile("w", dir=".") as f:
            f.write("abc\n")
            f.flush()

            self.assertTrue(os.path.exists(os.path.basename(f.name)))
            pp = cmonster.Preprocessor(
                "test.c",
                data="#include \"%s\"" % os.path.basename(f.name))
            tokens = [t for t in pp]
            self.assertEqual(1, len(tokens))
            self.assertEqual(cmonster.tok_identifier, tokens[0].token_id)
            self.assertEqual("abc", str(tokens[0]))


    def test_include_fileio(self):
        self.fail("not implemented")


    def test_include_fail(self):
        pp = cmonster.Preprocessor(
            "test.c",
            data="#include \"%s\"" % get_new_filename())
        with self.assertRaises(Exception):
            tokens = [t for t in pp]


if __name__ == "__main__":
    unittest.main()

