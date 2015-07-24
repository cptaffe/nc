#!/usr/bin/env bash

# compiler is overridable
if test -z ${CC}; then
	CC="cc"
fi

BIN="nc"
SRC="nc.c"

${CC} -nostdlib ${SRC} -o ${BIN}
