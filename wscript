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

    ctx.add_option(
        '--prefix',
        default=PREFIX,
        help="installation prefix [default: %r]"%PREFIX)

    return

def configure(ctx):
    ctx.load("hwaf-base")
    ctx.load('hwaf')

    ctx.find_program("llvm-config", var='LLVM-CONFIG')
    llvm_cfg = ctx.env['LLVM-CONFIG']
    ctx.check_with(
        ctx.check_cfg,
        "llvm",
        path=llvm_cfg,
        package="",
        uselib_store="llvm",
        args="--cppflags --libs --ldflags",
        )
    ctx.env['STLIB_clang-static'] = [
        "clang",
        "clangARCMigrate",
        "clangASTMatchers",
        "clangRewriteFrontend",
        "clangRewriteCore",
        "clangStaticAnalyzerCheckers",
        "clangStaticAnalyzerFrontend",
        "clangStaticAnalyzerCore",
        "clangTooling",

        "clangFrontendTool",
        "clangFrontend",
        "clangDriver",
        "clangSerialization",
        #"clangIndex",
        "clangParse",
        "clangSema",
        "clangEdit",
        "clangAnalysis",
        "clangCodeGen",
        "clangAST",
        "clangLex",
        "clangBasic",
        ]
    ctx.env['STLIBPATH_clang-static'] = ["/usr/lib/llvm"]
    
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
        linkflags = "-L/usr/lib/llvm -lpcrecpp -lyaml-cpp",
        use="llvm clang-static pthread yaml-cpp pcrepp pthread",
        includes = ". Transforms",
        )
