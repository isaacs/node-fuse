
CFLAGS = `pkg-config fuse --cflags --libs` -D__DARWIN_64_BIT_INO_T=0
NODEFLAGS = -I/usr/local/include/node

hello-ll-ev: hello-ll-ev.c
	gcc -Wall $(CFLAGS) $(NODEFLAGS) hello-ll-ev.c -o hello-ll-ev

hello-ll-mt: hello-ll-mt.c
	gcc -Wall $(CFLAGS) hello-ll-mt.c -o hello-ll-mt

hello: hello.c
	# gcc -Wall `pkg-config fuse --cflags --libs` -o hello hello.c -D_FILE_OFFSET_BITS=64
	gcc -Wall $(CFLAGS) hello.c -o hello

