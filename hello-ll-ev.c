/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` hello_ll.c -o hello_ll
*/

#define FUSE_USE_VERSION 26

#include <ev.h>

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <assert.h>

static const char *hello_str = "Hello World!\n";
static const char *hello_name = "hello";

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
        char *newp = realloc(b->p, b->size);
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

static struct fuse_lowlevel_ops hello_ll_oper = {
	.lookup		= hello_ll_lookup,
	.getattr	= hello_ll_getattr,
	.readdir	= hello_ll_readdir,
	.open		= hello_ll_open,
	.read		= hello_ll_read,
};


// ev-specific stuff below

static struct ev_loop *loop;
static ev_io channel_watcher;

static void channel_read 
  ( struct ev_loop *loop
  , ev_io *watcher
  , int revent
  )
{
  struct fuse_chan *channel = watcher->data;
  struct fuse_session *session = fuse_chan_session(channel);

  size_t bufferSize = fuse_chan_bufsize (channel);
  char buf[bufferSize];

  if (fuse_session_exited (session)) {
    ev_io_stop (loop, watcher);
    return;
  }

  struct fuse_chan *tmpch = channel;
  
  int recved;
  recved = fuse_chan_recv (&tmpch, buf, bufferSize);
  if (recved > 0)
    fuse_session_process (session, buf, recved, tmpch);
}


int main(int argc, char *argv[])
{
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *channel;
	char *mountpoint;
  int r = 0;

  r = fuse_parse_cmdline(&args, &mountpoint, NULL, NULL);
  if(r == -1) {
    fuse_opt_free_args(&args);
  	return 1;
  }

  channel = fuse_mount(mountpoint, &args);
  if(channel == NULL) {
    fuse_opt_free_args(&args);
    return 2;
  }

  struct fuse_session *session = 
    fuse_lowlevel_new(&args, &hello_ll_oper, sizeof(hello_ll_oper), NULL);

  if(session == NULL) {
    fuse_unmount(mountpoint, channel);
    fuse_opt_free_args(&args);
    return 1;
  }

  // SIGNAL HANDLERS
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&(sa.sa_mask));
  sa.sa_flags = 0;
  r = sigaction(SIGINT, &sa, NULL);
  assert(r == 0);
  r = sigaction(SIGTERM, &sa, NULL);
  assert(r == 0);

  fuse_session_add_chan(session, channel);

  // r = fcntl(fuse_chan_fd(channel), F_SETFL, O_NONBLOCK);
  // assert(r == 0);
  
  // set up the channel_watcher with ev
  memset(&channel_watcher, 0, sizeof(channel_watcher));
  fprintf(stderr, "about to ev init\n");
  ev_init(&channel_watcher, channel_read);
  fprintf(stderr, "about to ev io set\n");
  ev_io_set(&channel_watcher, fuse_chan_fd(channel), EV_READ);
  channel_watcher.data = channel;
  channel_watcher.fd = fuse_chan_fd(channel);
  fprintf(stderr, "fd = %d\n", channel_watcher.fd);
  
  struct ev_io *cw;
  cw = &channel_watcher;
  fprintf(stderr, "fd = %d\n", cw->fd);
  
  fprintf(stderr, "about to ev io start\n");
  // segfault here:
  ev_io_start(loop, &channel_watcher);
  
  // run the loop
  fprintf(stderr, "about to ev loop\n");
  ev_loop(loop, 0);

  fprintf(stderr, "exited loop\n");
  
  // cleanup
  // XXX Is this necessary here? If the loop exits, isn't it already "stopped"?
  ev_io_stop (loop, &channel_watcher);
  fuse_session_remove_chan(channel);
  fuse_session_destroy(session);
  fuse_unmount(mountpoint, channel);
  fuse_opt_free_args(&args);
  return 0;
}
