# Copyright 2020 Szymon Dziwak
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Example resources configuration xml
# <resources>
#     <resource path="../resources/main.glade" type="string" name="GLADE_MAIN"/>
#     <resource path="../resources/icon.png" type="bytes" name="ICON"/>
# </resources>

# Example usage
# python3 compile-resources.py resources.xml src/resources/.h

import os
import argparse
import struct
from xml.dom import minidom

parser = argparse.ArgumentParser(description="Embed text and binary files to C++ files.")
parser.add_argument('xml', metavar='X', type=str, help='Resources configuration file.')
parser.add_argument('output', metavar='O', type=str, help='Output header file.')
parser.add_argument('--namespace', '-n', dest='namespace', default=None)

args = parser.parse_args()

def escape(s):
    return s.translate(str.maketrans(
        {
            '\n': '\\n',
            '\r': '\\r',
            '\t': '\\t',
            '"': '\\"',
            '\\': '\\\\'
        })
    )

doc = minidom.parse(args.xml)

with open(args.output, 'w') as file:
    file.write("#pragma once\n")
    file.write("#include <string>\n\n")
    if args.namespace:
        file.write("namespace {} {{\n\n".format(args.namespace))
    for e in doc.getElementsByTagName("resource"):
        name_attr = e.getAttribute("name")
        type_attr = e.getAttribute("type")
        path_attr = e.getAttribute("path")
        if name_attr and path_attr:
            if not type_attr:
                type_attr = "bytes"
            if type_attr == "string":
                with open(path_attr, 'r') as f:
                    file.write("const std::string {name} = \"{value}\";\n\n".format(name=name_attr, value=escape(f.read())))
            elif type_attr == "bytes":
                with open(path_attr, 'rb') as f:
                    data = bytearray(f.read())
                file.write("const char {name}[] = {{{value}}};\n".format(name=name_attr, value=",".join([str(struct.unpack("b", struct.pack("B", b))[0]) for b in data])))
                file.write("const unsigned long int {name}_LENGTH = {value};\n\n".format(name=name_attr, value=str(len(data))))
    if args.namespace:
        file.write("\n\n}")
