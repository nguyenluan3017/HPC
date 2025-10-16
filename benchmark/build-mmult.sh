#!/usr/bin/env bash

SYS=$(uname)

rm -rf bin/*

case "$(uname)" in
    Darwin*)
        gcc -I$(brew --prefix openblas)/include \
            -L$(brew --prefix openblas)/lib \
            -o bin/mmult.darwin \
            mmult.c  \
            -lopenblas
        ;;
    Linux*)
        gcc -I/usr/local/include \
            -L/usr/local/lib \
            -o bin/mmult.linux \
            mmult.c \
            -lopenblas
        ;;
    *)
        echo "Unsupported system: '$SYS'"
    ;;
esac
