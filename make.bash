#!/usr/bin/env bash

# compiler is overridable
if test -z ${CC}; then
	CC="clang"
fi

BIN="nc"
SRC="nc.c"
FLAGS="-nostdlib -g -std=gnu99 -Wall -Wpedantic"

${CC} ${FLAGS} ${SRC} -o ${BIN}
