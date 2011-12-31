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

def cli_main():
    # First up, parse the command line arguments.
    import argparse
    description = "C Preprocessor with Python macros"
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument(
        "file", type=argparse.FileType("r"), nargs=1,
        help="the file to preprocess")
    parser.add_argument(
        "-I", action="append", dest="include_dirs")
    parser.add_argument(
        "-D", action="append", dest="defines")
    args = parser.parse_args()

    # Create the preprocessor.
    import sys
    from . import Parser

    parser = Parser(args.file[0])
    if args.include_dirs:
        for include_dir in args.include_dirs:
            parser.preprocessor.add_include_dir(include_dir)
    if args.defines:
        for define in args.defines:
            assign = define.find("=")
            if assign == -1:
                parser.preprocessor.define(define)
            else:
                name, value = define[:assign], define[assign+1:]
                parser.preprocessor.define(name, value)
    parser.preprocessor.preprocess()

