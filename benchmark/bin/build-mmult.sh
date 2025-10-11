#!/usr/bin/env bash

SYS=$(uname)

case "$(uname)" in
    Darwin*)
        gcc -I$(brew --prefix openblas)/include \
            -L$(brew --prefix openblas)/lib \
            -o mmult.macos \
            mmult.c  \
            -lopenblas
        ;;
    Linux*)
        gcc -I/usr/local/include \
            -L/usr/local/lib \
            -o mmult.linux \
            mmult.c \
            -lcblas
        ;;
    *)
        echo "Unsupported system: '$SYS'"
    ;;
esac
