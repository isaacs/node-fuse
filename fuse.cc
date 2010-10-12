#define FUSE_USE_VERSION 26

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fuse.h>
#include <fuse_lowlevel.h>
#include <errno.h>
#include <fcntl.h>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

using namespace node;
using namespace v8;

/*
fuse.mount(callbackObj, fuseArgs)
this will mount the filesystem with the callbacks on the callbackObj
it must implement all the functions in the fuse api, or goto hurty place.
*/

static Handle<Value> Mount (const Arguments&);
extern "C" void init (Handle<Object>);



static Handle<Value> Mount (const Arguments& args) {
  HandleScope scope;
  // convert to a fuse struct
  
}


extern "C" void init (Handle<Object> target) {
  HandleScope scope;
  NODE_SET_METHOD(target, "mount", Mount);
}

