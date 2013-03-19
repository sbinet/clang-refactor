# -*- python -*-

# imports ---------------------------------------------------------------------
import os
import os.path as osp

# globals ---------------------------------------------------------------------
top = '.'
out = '__build__'
PREFIX = 'install-area'
VERSION = '0.0.1' # FIXME: should take it from somewhere else
APPNAME = 'refactorial'

# imports ---------------------------------------------------------------------

# waf imports --
import waflib.Logs
import waflib.Utils
import waflib.Options
import waflib.Context
import waflib.Logs as msg

# functions -------------------------------------------------------------------

def pkg_deps(ctx):
    return

def options(ctx):
    ctx.load('hwaf')

    ctx.load('find_llvm')
    ctx.add_option(
        '--prefix',
        default=PREFIX,
        help="installation prefix [default: %r]"%PREFIX)

    return

def configure(ctx):
    ctx.load("hwaf-base")
    ctx.load('hwaf')

    ctx.load('find_llvm')
    ctx.find_llvm()
    ctx.find_libclang()

    ctx.check(features='cxx cxxprogram', lib="dl",       uselib_store="dl")
    ctx.check(features='cxx cxxprogram', lib='pcrecpp',  uselib_store='pcrecpp')
    ctx.check(features='cxx cxxprogram', lib='yaml-cpp', uselib_store='yaml-cpp')
    ctx.check(features='cxx cxxprogram', lib='pthread',  uselib_store='pthread')
    
    ctx.hwaf_configure()
    return

def build(ctx):
    ctx(
        features="cxx cxxprogram",
        name="refactorial",
        source=["main.cpp",
                "Refactoring.cpp",
                "Transforms/AccessorsTransform.cpp",
                "Transforms/ExtractParameterTransform.cpp",
                "Transforms/FunctionRenameTransform.cpp",
                "Transforms/IdentityTransform.cpp",
                "Transforms/MethodMoveTransform.cpp",
                "Transforms/RecordFieldRenameTransform.cpp",
                "Transforms/Transforms.cpp",
                "Transforms/TypeRenameTransform.cpp",
                ],
        target="clang-refactorial",
        cxxflags = "-std=c++11",
        use="LLVM-static clang-static yaml-cpp pcrecpp pthread",
        includes = ". Transforms",
        )
