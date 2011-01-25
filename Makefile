
CFLAGS = `pkg-config fuse --cflags --libs` -D__DARWIN_64_BIT_INO_T=0
CFLAGS+=  -Wall -Wextra -pedantic -ggdb3
NODEFLAGS = `pkg-config nodejs --libs --cflags`

all: hello-ll-ev hello-ll-mt hello
	@true

hello-ll-ev: hello-ll-ev.c
	gcc -Wall $(CFLAGS) $(NODEFLAGS) -lev hello-ll-ev.c -o hello-ll-ev

hello-ll-mt: hello-ll-mt.c
	gcc -Wall $(CFLAGS) hello-ll-mt.c -o hello-ll-mt

hello: hello.c
	# gcc -Wall `pkg-config fuse --cflags --libs` -o hello hello.c -D_FILE_OFFSET_BITS=64
	gcc -Wall $(CFLAGS) hello.c -o hello

clean:
	rm hello hello-ll-mt hello-ll-ev

.PHONY: all clean
