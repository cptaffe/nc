#!/usr/bin/env bash

# compiler is overridable
if test -z ${CC}; then
	CC="clang"
fi

BIN="nc"
SRC="nc.c"
FLAGS="-nostdlib -g --std=c99 -Weverything -Werror \
-Wno-non-literal-null-conversion -Wno-gnu-empty-initializer -Wno-padded"

${CC} ${FLAGS} ${SRC} -o ${BIN}
