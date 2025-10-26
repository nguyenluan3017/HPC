CC = gcc
CFLAGS = -Wall
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	INCLUDES = -I/usr/include/x86_64-linux-gnu/openblas64-pthread
	LIBS = -lpthread -lopenblas -lm
endif
ifeq ($(UNAME_S),Darwin)
	INCLUDES := -I$(shell brew --prefix openblas)/include
	LIBS = -lpthread -L$(shell brew --prefix openblas)/lib -lopenblas
endif

norm: norm.c
	mkdir -p bin/
	$(CC) $(INCLUDES) $(CFLAGS) norm.c -o ./bin/norm $(LIBS)

benchmark: benchmark.go norm
	go run benchmark.go --output-dir $(shell echo $(OUTPUT_DIR))

test: bin/norm
	bin/norm --help

clean: ./bin
	rm -rfv ./bin
