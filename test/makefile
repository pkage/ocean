all: recv

clean:
	rm -f send recv static

send: clean
	clang send.c -o send -std=c11 -g

recv: clean
	clang recv.c -o recv -std=c11 -g

static: clean
	clang static.c -o static -std=c11 -g
