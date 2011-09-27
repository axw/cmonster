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
import cmonster.ast
import os
import unittest

class TestParser(unittest.TestCase):
    def test_parse_function(self):
        pp = cmonster.Parser("test.c", data="char func(int x) {return 123L;}")
        result = pp.parse()
        self.assertIsNotNone(result)

        # There's always a builtin typedef declaration first (should we report
        # this to the user?), so we should have two declarations in total, with
        # the second one being the function.
        tu = result.translation_unit
        decls = [d for d in tu.declarations]
        self.assertEqual(2, len(decls))
        self.assertIsInstance(decls[1], cmonster.ast.FunctionDecl, decls[1])
        func_decl = decls[1]

        # Check the function name and return type.
        self.assertEqual("func", func_decl.name)
        func_type = func_decl.type
        self.assertIsInstance(func_type.type, cmonster.ast.FunctionType)
        self.assertIsInstance(
            func_type.type.result_type.type,
            cmonster.ast.BuiltinType)
        self.assertEqual(
            cmonster.ast.BuiltinType.Char_S,
            func_type.type.result_type.type.kind)

        # Check the parameter type.
        self.assertEqual(1, len(func_decl.parameters))
        self.assertEqual("x", func_decl.parameters[0].name)
        self.assertIsInstance(
            func_decl.parameters[0].type.type,
            cmonster.ast.BuiltinType)
        self.assertEqual(
            cmonster.ast.BuiltinType.Int,
            func_decl.parameters[0].type.type.kind)

        # TODO check the body...
        body = func_decl.body
        self.assertIsNotNone(body)
        self.assertEqual(1, len(body))
        self.assertIsInstance(body[0], cmonster.ast.ReturnStatement)
        return_value = body[0].return_value
        self.assertEqual(
            cmonster.ast.BuiltinType.Long,
            return_value.subexpr.type.type.kind)
        self.assertEqual(123, return_value.subexpr.value)


    def test_invalid_toplevel_decl(self):
        pp = cmonster.Parser(
            "test.c",
            data="abc")
        with self.assertRaises(Exception):
            parser.parse()


if __name__ == "__main__":
    unittest.main()

