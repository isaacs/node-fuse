
hello: hello.c
	# gcc -Wall `pkg-config fuse --cflags --libs` -o hello hello.c -D_FILE_OFFSET_BITS=64
	gcc -Wall `pkg-config fuse --cflags --libs` hello.c -o hello
