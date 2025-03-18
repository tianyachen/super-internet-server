CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: baseline cgi fast_server super_server

fast_server: fast_server.c csapp.o sbuf.o
	$(CC) $(CFLAGS) -rdynamic -o fast_server fast_server.c csapp.o $(LIB) -ldl

super_server: super_server.c csapp.o sbuf.o
	$(CC) $(CFLAGS) -rdynamic -o super_server super_server.c csapp.o sbuf.o $(LIB) -ldl

baseline: baseline.c csapp.o
	$(CC) $(CFLAGS) -o baseline baseline.c csapp.o $(LIB)

csapp.o:
	$(CC) $(CFLAGS) -c csapp.c

sbuf.o:
	$(CC) $(CFLAGS) -c sbuf.c

cgi:
	(cd cgi-bin; make)

clean:
	rm -f *.o baseline fast_server super_server *~
	(cd cgi-bin; make clean)

