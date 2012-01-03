# cmonster

cmonster is a Python wrapper for the [Clang](http://clang.llvm.org/) C++
parser.

As well as providing standard preprocessing/parsing capabilities, cmonster adds
support for:

* Inline Python macros;
* Programmatic #include handling; and
* (Currently rudimentary) source-to-source translation.

## Documentation

There's no proper documentation yet; I'll get to it eventually. In the mean
time...

### Inline Python macros

You can define inline Python macros like so:

    py_def(function_name(arg1, arg2, *args, **kwdargs))
        return [list_of_tokens]
    py_end

Python macros must return a string, which will be tokenized, or a sequence of
tokens (e.g. some combination of the input arguments). Python macros are used
exactly the same as ordinary macros. e.g.

    py_def(REVERSED(token))
        return "".join(reversed(str(token)))
    py_end

    int main() {
        printf("%s\n", REVERSED("Hello, World!"));
    }

### Source-to-source translation

Source-to-source translation involves parsing a C++ file, generating an AST;
then traversing the AST, and making modifications in a _rewriter_. The rewriter
object maintains a history of changes, and can eventually be told to dump the
modified file to a stream.

For example, here's a basic example of how to make an insertion at the top of
each top-level function declaration's body:

    # Create a parser, and a rewriter. Parse the code.
    parser = cmonster.Parser(filename)
    ast = parser.parse()
    rewriter = cmonster.Rewriter(ast)

    # For each top-level function declaration, insert a statement at the top of
    # its body.
    for decl in ast.translation_unit.declarations:
        if decl.location.in_main_file and \
           isinstance(decl, cmonster.ast.FunctionDecl):
            insertion_loc = decl.body[0]
            rewriter.insert(insertion_loc, 'printf("Tada!\\n");\n')

    # Finally, dump the result.
    rewriter.dump(sys.stdout)


## Installation

cmonster requires [Python 3.2](http://python.org/download/releases/3.2.2/),
[LLVM/Clang 3.0](http://llvm.org/releases/download.html#3.0), and
[Cython](http://cython.org/#download), so first ensure you have them
installed.

Now you can use
[easy\_install](http://packages.python.org/distribute/easy_install.html) to
install cmonster, like so: `easy_install cmonster`. This will download and
install a prebuilt binary distribution of cmonster.

If you wish to build cmonster yourself, either pull down the git repository, or
grab the source distribution from
[cmonster's PyPI page](http://pypi.python.org/pypi/cmonster/). When building,
make sure you have LLVM 3.0's llvm-config in your execution path. To verify
this, run `llvm-config --version`; you should expect to see `3.0` output. To
build from source, simply run `python3.2 setup.py install`.

