
#include "def.h"
#include "heap.cc"
// Syscall, Mem, String depend on def.h
#include "syscall.cc"
#include "symb.cc"
// String, Mem depend on syscall.c
#include "mem.cc"
// String mem.c

/* Netcat utility
 * written with no standard library,
 * for Linux only.
 * hopefully also a cleaner nc implementation
 */

// start symbol
extern "C" void _start(void);
extern "C" void init(void);
extern "C" void fini(void);

extern "C" void _start() {
	init();
	int *arr[10];

	int i;
	for (i = 0; i < 10; i++) {
		arr[i] = new int[0x1000];
	}

	for (i = 0; i < 10; i++) {
		delete arr[i];
	}

	fini();
	syscall::call(syscall::Call::kExit, 1, 0, 0, 0, 0, 0);
}
