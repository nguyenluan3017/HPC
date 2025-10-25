CC = gcc
CFLAGS = -Wall
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	INCLUDES = -I/usr/include/x86_64-linux-gnu/openblas64-pthread/
	LIBS = -lpthread -lopenblas
endif
ifeq ($(UNAME_S),Darwin)
	INCLUDES := -I$(shell brew --prefix openblas)/include
	LIBS = -lpthread -L$(shell brew --prefix openblas)/lib -lopenblas
endif

norm:
	mkdir -p bin/
	$(CC) $(INCLUDES) $(CFLAGS) norm.c -o ./bin/norm $(LIBS)

test: bin/norm
	bin/norm --help

clean:
	rm -rfv ./bin
