import SCons.Script
import os, sys
import shutil
import string

# specify include path and source files to be used
CPPPATH = ''
CXXFLAGS = '-std=c++0x -O3 -Wall -Wfatal-errors'
LIBS = []

SUFLEM_LIB_SRC = ['Model.cpp']
SUFLEM_BIN_SRC = ['suflem.cpp']

# set up SwigScanner
SWIGScanner = SCons.Scanner.ClassicCPP(
    "SWIGScan",
    ".i",
    "CPPPATH",
    '^[ \t]*[%,#][ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")'
)


env = Environment(
    ENV = os.environ,
    CPPPATH=CPPPATH,
    CXXFLAGS=CXXFLAGS,
    LIBS=LIBS,
    SHLIBPREFIX='')

env.SharedLibrary('suflem', SUFLEM_LIB_SRC)
env.StaticLibrary('suflem', SUFLEM_LIB_SRC)
env.Program('suflem', SUFLEM_LIB_SRC + SUFLEM_BIN_SRC)
