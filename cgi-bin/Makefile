CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder mylib.so

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

mylib.so: myadder.c
	$(CC) $(CFLAGS) -shared -fpic -o mylib.so myadder.c 

clean:
	rm -f adder *.so *~