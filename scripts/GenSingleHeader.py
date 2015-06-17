#!/usr/bin/python3
# Create a single header file from many sources
# (C) 2015 Niall Douglas http://www.nedprod.com/
# Created: June 2015

import sys, os, re

if len(sys.argv)<2:
    print("Usage: "+sys.argv[0]+" [-Iincludepath...] header1 [header2...]", file=sys.stderr)
    sys.exit(1)

includepaths=[]
headers=[]
for arg in sys.argv[1:]:
    if arg[:2]=="-I":
        includepaths.append(os.path.abspath(arg[2:]))
    else:
        headers.append(arg)
#print(includepaths)

def find_header(header):
    if os.path.exists(header):
        return header
    for includepath in includepaths:
        if os.path.exists(os.path.join(includepath, header)):
            return os.path.join(includepath, header)
    return None

headers_seen={}
def parse_header(indent, header_path):
    indentstring=" ".ljust(indent)
    currentpath=os.getcwd()
    #print(indentstring+currentpath+": Now parsing "+header_path)
    print("/* Begin "+os.path.join(currentpath, header_path)+" */")
    lineno=0
    with open(header_path, "rt") as ih:
        lastlinewasempty=False
        for line in ih:
            line=line.strip()
            lineno+=1
            if lastlinewasempty and len(line)==0:
                continue;
            lastlinewasempty=(len(line)==0)
            include=None
            if "include" in line:
                is_include=re.match("""\s*#\s*include\s*["<](.*)[">]""", line)
                if is_include is not None:
                    include=is_include.group(1)
                else:
                    # #include BOOST_BINDLIB_INCLUDE_STL11(bindlib, BOOST_AFIO_V1_STL11_IMPL, atomic)
                    is_include=re.match("""\s*#\s*include\s*BOOST_BINDLIB_INCLUDE_STL11\((.*),.*, (.*)\)""", line)
                    if is_include:
                        include=os.path.join(is_include.group(1), "bind/stl11/std", is_include.group(2))
                    else:
                      is_include=re.match("""\s*#\s*include\s*BOOST_BINDLIB_INCLUDE_STL1z\((.*),.*, (.*)\)""", line)
                      if is_include:
                          if is_include.group(2)=="filesystem":
                              include=os.path.join(is_include.group(1), "bind/stl1z/std/filesystem")
                          elif is_include.group(2)=="networking":
                              include=os.path.join(is_include.group(1), "bind/stl1z/asio/networking")
            if include is not None:
                #print(indentstring+"   Found #include "+is_include.group(1)+" at line "+str(lineno)+" "+line)
                try:
                    os.chdir(os.path.join(currentpath, os.path.dirname(header_path)))
                    sub_header_path=find_header(include)
                    if sub_header_path is not None:
                        fullincludepath=os.path.abspath(sub_header_path)
                        if fullincludepath not in headers_seen:
                            #print(indentstring+"   Not seen header "+fullincludepath+" before, parsing")
                            headers_seen[fullincludepath]=None
                            parse_header(indent+4, sub_header_path)
                finally:
                    os.chdir(currentpath)
                continue
            print(line)
    print("/* End "+os.path.join(currentpath, header_path)+" */\n")

for header in headers:
    header_path=find_header(header)
    parse_header(0, header_path)