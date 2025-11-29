CC = gcc
CFLAGS = -Wall -Werror -fPIC

all: lib1.so lib2.so program1 program2

lib1.so: lib1.c
	$(CC) $(CFLAGS) -shared -o lib1.so lib1.c

lib2.so: lib2.c
	$(CC) $(CFLAGS) -shared -o lib2.so lib2.c

program1: program1.c lib1.so
	$(CC) program1.c -L. -l1 -o program1 -Wl,-rpath,.

program2: program2.c lib1.so lib2.so
	$(CC) program2.c -ldl -o program2

clean:
	rm -f *.so program1 program2
