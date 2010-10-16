srcdir = "."
blddir = "build"
VERSION = "0.0.1"

import sys

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check_cfg(atleast_pkgconfig_version="0.0.0")
  
  conf.check_cfg( package="nodejs"
                , args="--cflags --libs"
                , mandatory=True
                )
  conf.check_cfg( package="fuse"
                , args="--cflags --libs"
                , mandatory=True
                , uselib_store="FUSE"
                , msg="checking for fuse"
                )
  conf.check( lib="fuse"
            , uselib_store="FUSE"
            , mandatory=True
            )

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  # obj.cxxflags = ["-Wall"]
  obj.uselib = "FUSE"
  # obj.target = "nas"
  # obj.source = "nas.cc"
  obj.target = "hello-ll"
  obj.source = "hello-ll-node.cc"
  
  obj.cxxflags = [ "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE" ]
  if sys.platform.startswith("darwin"):
    obj.cxxflags.append("-D__DARWIN_64_BIT_INO_T=0")
