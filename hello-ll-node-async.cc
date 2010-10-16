/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` hello_ll.c -o hello_ll
*/

#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <fuse_lowlevel.h>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

using namespace v8;
using namespace std;
using namespace node;

static const char *hello_str = "Hello World!\n";
static const char *hello_name = "hello";
static struct fuse_lowlevel_ops hello_ll_oper;

struct hello_req {
  Persistent<Function> cb;
  int argc;
  int retval;
  char **argv; // mountpoint is shoved onto this.
};


static Handle<Value> Mount (const Arguments&);
static Handle<Value> MountAsync (const Arguments&);
extern "C" void init (Handle<Object>);
int hello_main (int, char **);
static int EIO_Mount (eio_req *);
static int Mount_After (eio_req *);

static Handle<Value> MountAsync (const Arguments& args) {
  HandleScope scope;
  const char *usage = "usage: mount(mountpoint [, fuseargs])";
  if (args.Length() < 2
    || args.Length() > 3
    || !args[0]->IsString()
    || (args.Length() == 3 && !args[1]->IsArray())
    || (! args[args.Length() - 1]->IsFunction())
  ) {
    return ThrowException(Exception::Error(String::New(usage)));
  }
  Local<Function> cb = Local<Function>::Cast(args[args.Length() - 1]);
  Handle<Array> fuseArgs;
  if (args.Length() == 2) {
    fuseArgs = Array::New(0);
  } else {
    fuseArgs = Handle<Array>::Cast(args[1]);
  }

  char **argv;
  int argc = 1 + fuseArgs->Length();

  // mountpoint is the first argument.
  String::Utf8Value mp(args[0]);
  argv = (char **)malloc(argc * sizeof(char *) + sizeof(void *));
  assert(argv);
  argv[0] = (char *)malloc(sizeof(char) * (mp.length() + 1));
  assert(argv[0]);
  strncpy(argv[0], *mp, mp.length() + 1);
  for (int i = 1; i < argc; i ++) {
    String::Utf8Value fa(fuseArgs->Get(i - 1)->ToString());
    argv[i] = (char *)malloc(sizeof(char) * fa.length() + 1);
    assert(argv[i]);
    strncpy(argv[i], *fa, fa.length() + 1);
  }
  fprintf(stderr, "argc: %i\n", argc);
  fprintf(stderr, "args:\n");
  for (int i = 1; i < argc; i ++) {
    fprintf(stderr, "%i: %s\n", i, argv[i]);
  }
  argv[argc] = NULL;
  // create the hello request.
  // we're going to do something like hello_main(argc, argv)
  hello_req *hr = (hello_req *)malloc(sizeof(struct hello_req));
  assert(hr);
  memset(hr, 0, sizeof(struct hello_req));
  hr->cb = Persistent<Function>::New(cb);
  hr->argc = argc;
  hr->argv = argv;

  eio_custom(EIO_Mount, EIO_PRI_DEFAULT, Mount_After, hr);
  ev_ref(EV_DEFAULT_UC);
  return Undefined();
}

// this function happens on the thread pool
// doing v8 things in here will make bad happen.
static int EIO_Mount (eio_req *req) {
  struct hello_req * hr = (struct hello_req *)req->data;
  hr->retval = hello_main(hr->argc, hr->argv);
  return 0;
}

static int Mount_After (eio_req *req) {
  HandleScope scope;
  ev_unref(EV_DEFAULT_UC);
  struct hello_req * hr = (struct hello_req *)req->data;
  
  // it's been unmounted, either by us or someone else.
  Local<Value> argv[1];
  argv[0] = hr->retval ? Exception::Error(Number::New(hr->retval)->ToString())
                       : Local<Value>::New(Null());
  TryCatch try_catch;
  hr->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  if (try_catch.HasCaught()) FatalException(try_catch);
  hr->cb.Dispose();
  free(hr);
  return 0;
}


static int hello_stat(fuse_ino_t ino, struct stat *stbuf)
{
	stbuf->st_ino = ino;
	switch (ino) {
	case 1:
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		break;

	case 2:
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
		break;

	default:
		return -1;
	}
	return 0;
}

static void hello_ll_getattr(fuse_req_t req, fuse_ino_t ino,
			     struct fuse_file_info *fi)
{
	struct stat stbuf;

	(void) fi;

	memset(&stbuf, 0, sizeof(stbuf));
	if (hello_stat(ino, &stbuf) == -1)
		fuse_reply_err(req, ENOENT);
	else
		fuse_reply_attr(req, &stbuf, 1.0);
}

static void hello_ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;

	if (parent != 1 || strcmp(name, hello_name) != 0)
		fuse_reply_err(req, ENOENT);
	else {
		memset(&e, 0, sizeof(e));
		e.ino = 2;
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		hello_stat(e.ino, &e.attr);

		fuse_reply_entry(req, &e);
	}
}

struct dirbuf {
	char *p;
	size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
		       fuse_ino_t ino)
{
	struct stat stbuf;
	size_t oldsize = b->size;
	b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
  char *newp = (char *)realloc(b->p, b->size);
  if (!newp) {
    fprintf(stderr, "*** fatal error: cannot allocate memory\n");
    abort();
  }
	b->p = newp;
	memset(&stbuf, 0, sizeof(stbuf));
	stbuf.st_ino = ino;
	fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
			  b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
			     off_t off, size_t maxsize)
{
	if (off < bufsize)
		return fuse_reply_buf(req, buf + off,
				      min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}

static void hello_ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
			     off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	if (ino != 1)
		fuse_reply_err(req, ENOTDIR);
	else {
		struct dirbuf b;

		memset(&b, 0, sizeof(b));
		dirbuf_add(req, &b, ".", 1);
		dirbuf_add(req, &b, "..", 1);
		dirbuf_add(req, &b, hello_name, 2);
		reply_buf_limited(req, b.p, b.size, off, size);
		free(b.p);
	}
}

static void hello_ll_open(fuse_req_t req, fuse_ino_t ino,
			  struct fuse_file_info *fi)
{
	if (ino != 2)
		fuse_reply_err(req, EISDIR);
	else if ((fi->flags & 3) != O_RDONLY)
		fuse_reply_err(req, EACCES);
	else
		fuse_reply_open(req, fi);
}

static void hello_ll_read(fuse_req_t req, fuse_ino_t ino, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	assert(ino == 2);
	reply_buf_limited(req, hello_str, strlen(hello_str), off, size);
}




int hello_main (int argc, char **argv)
{
  fprintf(stderr, "hello_main with %d args\n", argc);
  for (int i = 0; i < argc; i ++) {
    fprintf(stderr, "%d: %s\n", i, argv[i]);
  }

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  fprintf(stderr, "fused args %d\n", args);

	struct fuse_chan *ch;
	char *mountpoint;
	int err = -1;
  int ret = fuse_parse_cmdline(&args, &mountpoint, NULL, NULL);
  fprintf(stderr, "fused cmdline %d (0 is good)\n", ret);
  fprintf(stderr, "about to mount to %s\n", mountpoint);
  for (int i = 0; i < argc; i ++) {
    fprintf(stderr, "%d: %s\n", i, argv[i]);
  }
  ch = fuse_mount(argv[0], &args);
  fprintf(stderr, "fuse mounted to channel: %d\n", ch);
  assert(ch);

	if (ret != -1 && ch != NULL) {
    fprintf(stderr, "fuse mounted\n");
		struct fuse_session *se;

		se = fuse_lowlevel_new(&args, &hello_ll_oper,
                           sizeof(hello_ll_oper), NULL);
    fprintf(stderr, "fused new lowlevel %d\n", se);
		if (se != NULL) {
			if (fuse_set_signal_handlers(se) != -1) {
        fprintf(stderr, "fused signal handlers\n");
				fuse_session_add_chan(se, ch);
        fprintf(stderr, "fused channel to session, entering loop\n");
				err = fuse_session_loop(se);
        fprintf(stderr, "out of loop: %d\n", err);
				fuse_remove_signal_handlers(se);
        fprintf(stderr, "removed sig handlers\n");
				fuse_session_remove_chan(ch);
        fprintf(stderr, "removed channel\n");
			}
			fuse_session_destroy(se);
      fprintf(stderr, "destroyed session\n");
		}
		fuse_unmount(mountpoint, ch);
    fprintf(stderr, "unmounted\n");
	}
	fuse_opt_free_args(&args);
  fprintf(stderr, "freed args\n");

	return err ? 1 : 0;
}


extern "C" void init (Handle<Object> target) {
  HandleScope scope;

  hello_ll_oper.lookup  = hello_ll_lookup;
  hello_ll_oper.getattr	= hello_ll_getattr;
  hello_ll_oper.readdir	= hello_ll_readdir;
  hello_ll_oper.open    = hello_ll_open;
  hello_ll_oper.read    = hello_ll_read;

  NODE_SET_METHOD(target, "mount", MountAsync);
}