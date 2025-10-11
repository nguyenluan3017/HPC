#!/usr/bin/env bash

SYS=$(uname)

if [[ "$SYS" == Darwin* ]]; then
    gcc -I$(brew --prefix openblas)/include \
        -L$(brew --prefix openblas)/lib \
        -o mmult.macos \
        mmult.c  \
        -lopenblas
elif [[ "$SYS" == Linux* ]]; then
    gcc -I/usr/local/include \
        -L/usr/local/lib \
        -o mmult.linux
        mmult.c \
        -lcblas
else
    echo "Unsupported system: '$SYS'"
fi
