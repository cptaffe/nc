
#include "def.h"
#include "heap.c"
// Syscall, Mem, String depend on def.h
#include "syscall.c"
// String, Mem depend on syscall.c
#include "mem.c"
// String mem.c
#include "string.c"

// Netcat utility
// Written with no standard library,
// for Linux only.
// Hopefully also a cleaner nc implementation

typedef struct {
	int fd;
} Sock;

typedef enum {
	// Linux protocols
	kSockDomainLocal,
	kSockDomainIPv4,
	kSockDomainIPv6,
	kSockDomainIPX,
	kSockDomainNetlink,
	kSockDomainX25,
	kSockDomainAX25,
	kSockDomainAtmPvc,
	kSockDomainAppleTalk,
	kSockDomainPacket,
	kSockDomainAlg
} SockDomain;

typedef enum {
	kSockTypeStream    = 1,  // stream (connection) socket
	kSockTypeDgram     = 2,  // datagram (conn.less) socket
	kSockTypeRaw       = 3,  // raw socket
	kSockTypeRDM       = 4,  // reliably-delivered message
	kSockTypeSeqPacket = 5,  // sequential packet socket
	kSockTypeDCCP      = 6,  // Datagram Congestion Control Protocol socket
	kSockTypePacket    = 10, // linux specific way of getting packets
						 // at the dev level. For writing rarp and
						 // other similar things on the user level.
	// TODO(cptaffe): add xorable flags
} SockType;

// Castable representation of both v4 and v6.
typedef struct {
	u16 family;
	u8 data[14];
} SockAddr;

typedef struct {
	u32 addr;
} SockInetAddrv4;

typedef struct {
	u8 addr[16];
} SockInetAddrv6;

typedef struct {
	u16 family, port;
	SockInetAddrv4 addr;
	u8 zero[8]; // zeroed section
} SockAddrv4;

typedef struct {
	u16 family, port;
	u32 flowInfo;
	SockInetAddrv6 addr;
	u32 scopeId;
} SockAddrv6;

void makeSockAddrv4(SockAddrv4 *sockAddr, u16 port) {
	// port is in network order
	upendMem(&port, sizeof(port));
	*sockAddr = (SockAddrv4){
		.family = kSockDomainIPv4,
		.port   = port
	};
}

void makeSockAddrv6(SockAddrv6 *sockAddr) {
	*sockAddr = (SockAddrv6){
		.family = kSockDomainIPv6
	};
}

// Assumes there is only one protocol for each, namely 0
void makeSock(Sock *sock, SockDomain domain, SockType type) {
	zeroMem(sock, sizeof(sock));
	sock->fd = syscall(kSyscallSocket, (u64[]){(u64) domain, (u64) type, 0, 0, 0, 0});
}

// Sanity check
static bool _sockOk(Sock *sock) {
	return sock->fd != nil;
}

Sock *connectSock(Sock *sock, SockAddr *addr, u64 len) {
	if (!_sockOk(sock)) return nil;
	int err = syscall(sock->fd, (u64[]){(u64) addr, len, 0, 0, 0, 0});
	if (err != 0) return nil;
}

void printNum(u64 num, int base) {
	char buf[256];
	// If buffer is too short, memBitString returns nil
	writeString(appendString(numAsString((string){
		.buf  = buf,
		.size = sizeof buf,
	}, num, base), '\n'), 1);
}

// Utility hex dump function.
// Prints quadwords in hex separated by a dot
// If less than a quadword exists or it is not aligned.
void hexDump(void *mem, u64 size) {
	int bytes = size % sizeof(u64);
	int big = size / sizeof(u64);

	char buf[0x1000];
	string str = {
		.buf  = buf,
		.size = sizeof buf,
	};

	int i;
	for (i = 0; i < big; i++) {
		appendString(numAsString(str, ((u64*)mem)[i], 16), '.');
	}

	for (i = 0; i < bytes; i++) {
		numAsString(str, ((u8*)&((u64*)mem)[big])[i], 16);
	}

	writeString(appendString(str, '\n'), 1);
}

// Implement example IntHeap with heap.c

// String serves as IntHeap
typedef struct {
	int array[10];
	int size;
} IntHeap;

void swapIntHeap(void *v, int i, int j) {
	IntHeap *h = ((IntHeap *) v);
	int b = h->array[i];
	h->array[i] = h->array[j];
	h->array[j] = b;
}

int lenIntHeap(void *v) {
	return ((IntHeap *) v)->size;
}

bool lessIntHeap(void *v, int i, int j) {
	IntHeap *h = ((IntHeap *) v);
	return h->array[i] < h->array[j];
}

void pushIntHeap(void *v, void *x) {
	IntHeap *h = ((IntHeap *) v);
	h->array[h->size] = (int) (u64) x;
	h->size++;
}

void *popIntHeap(void *v) {
	IntHeap *h = ((IntHeap *) v);
	h->size--;
	return (void *) (u64) h->array[h->size];
}

Heap asHeapIntHeap(IntHeap h) {
	// Fill in Heap interface
	return (Heap){
		.heap = &h,
		.i = (Heapable){
			.sort = (Sortable){
				.len  = lenIntHeap,
				.less = lessIntHeap,
				.swap = swapIntHeap
			},
			.pop  = popIntHeap,
			.push = pushIntHeap
		}
	};
}

void _start() {
	Heap h = asHeapIntHeap((IntHeap){
		.array = {1, 5, 2, 8},
		.size  = 4
	});
	initHeap(&h);
	pushHeap(&h, (void *) (u64) 7);

	while (lenIntHeap(h.heap) > 0) {
		printNum((int) (u64) popHeap(&h), 10);
	}

	syscall(kSyscallExit, (u64[]){1, 0, 0, 0, 0, 0});
}
