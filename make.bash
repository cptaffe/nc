#!/usr/bin/env bash

# compiler is overridable
if test -z ${CXX}; then
	CXX="clang++"
fi

BIN="nc"
SRC="nc.cc"
FLAGS="-nostdlib -fno-rtti -fno-exceptions -g -std=c++11 -Weverything \
-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables \
-Wno-missing-prototypes -Wno-global-constructors -Wno-exit-time-destructors \
-Wno-missing-variable-declarations"
# Since I include all .cc files, weak vtables will exits.
# But there is only one translation unit, so who cares.
# Also, there will be no function prototypes because we include .cc files.

${CXX} ${FLAGS} ${SRC} -o ${BIN}
