#!/usr/bin/env bash

if test -z ${CC}; then
	CC="cc"
fi

SRC="nc.c"
BIN="nc"

${CC} -nostdlib ${SRC} -o ${BIN}

