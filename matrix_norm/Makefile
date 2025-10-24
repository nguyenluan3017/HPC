CC = gcc
CFLAGS = -Wall
LIBS = -lpthread -lopenblas
INCLUDES = -I/usr/include/x86_64-linux-gnu/openblas64-pthread/

norm:
	mkdir -p bin/
	$(CC) $(INCLUDES) $(CFLAGS) norm.c -o ./bin/norm $(LIBS)

test: bin/norm
	bin/norm --help

clean:
	rm -rfv ./bin
