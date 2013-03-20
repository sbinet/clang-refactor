# -*- python -*-

# imports ---------------------------------------------------------------------
import os
import os.path as osp

import waflib.Logs as msg

# functions -------------------------------------------------------------------

def pkg_deps(ctx):
    return

def options(ctx):
    ctx.load('find_python')
    ctx.load('find_boost')
    ctx.load('find_llvm')
    return

def configure(ctx):

    ctx.load('find_python')
    ctx.load('find_boost')
    ctx.find_boost()
    
    ctx.load('find_llvm')
    ctx.find_llvm()
    ctx.find_libclang()

    ctx.check(features='cxx cxxprogram', lib="dl",       uselib_store="dl")
    ctx.check(features='cxx cxxprogram', lib='pcrecpp',  uselib_store='pcrecpp')
    ctx.check_with(
        ctx.check_cfg,
        "yaml-cpp",
        package="yaml-cpp",
        uselib_store="yaml-cpp",
        args='--cflags --libs')
    ctx.check(features='cxx cxxprogram', lib='pthread',  uselib_store='pthread')
    return

def build(ctx):
    ctx(
        features="cxx cxxprogram",
        name="clang-refactor",
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
        target="clang-refactor",
        cxxflags = "-std=c++11",
        use="LLVM-static clang-static yaml-cpp pcrecpp pthread boost",
        includes = ". Transforms",
        )
    return

## EOF ##
    
