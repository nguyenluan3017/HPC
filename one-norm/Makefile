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

bin/norm: norm.c
	mkdir -p bin/
	$(CC) $(INCLUDES) $(CFLAGS) norm.c -o ./bin/norm $(LIBS)

bin/benchmark: benchmark.go
	mkdir -p ./bin
	go build -o bin/benchmark benchmark.go

benchmark: bin/benchmark bin/norm
	mkdir -p ./out
	./bin/benchmark \
		-output-dir ./out \
		-exec ./bin/norm

plot-results: ./results/serial.yaml ./results/threaded.yaml	bin/benchmark
	./bin/benchmark \
		-results-path ./results
		-plot

clean: ./bin
	rm -rfv ./bin
