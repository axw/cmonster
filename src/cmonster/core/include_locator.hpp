/*
Copyright (c) 2011 Andrew Wilkins <axwalk@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _CMONSTER_CORE_INCLUDE_LOCATOR_HPP
#define _CMONSTER_CORE_INCLUDE_LOCATOR_HPP

#include <string>

namespace cmonster {
namespace core {

/**
 * Abstract base class for locating include files externally. The preprocessor
 * will consult an implementation of IncludeLocator when its internal lookup
 * fails.
 */
class IncludeLocator
{
public:
    virtual ~IncludeLocator() {}

    /**
     * Locate the absolute path of an #include filename, which is presented in
     * either the angled or quoted form (i.e. <filename> or "filename").
     *
     * @param filename The filename, in angled or quoted form, to locate.
     * @param absolute_path The resultant absolute path, if located.
     * @return True if the include could be resolved, else false.
     */
    virtual bool locate(std::string const& filename,
                        std::string &absolute_path) const = 0;
};

}}

#endif

