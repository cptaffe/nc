
#include "def.h"
#include "heap.c"
// Syscall, Mem, String depend on def.h
#include "syscall.c"
// String, Mem depend on syscall.c
#include "mem.c"
// String mem.c
#include "string.c"

/* Netcat utility
 * written with no standard library,
 * for Linux only.
 * hopefully also a cleaner nc implementation
 */

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

/* Socket utility methods
 *
 */
void makeSockAddrv4(SockAddrv4 *sockAddr, u16 port);
void makeSockAddrv6(SockAddrv6 *sockAddr);
void makeSock(Sock *sock, SockDomain domain, SockType type);
Sock *connectSock(Sock *sock, SockAddr *addr, u64 len);

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
	sock->fd = (int) syscall(kSyscallSocket, (u64[]){(u64) domain, (u64) type, 0, 0, 0, 0});
}

/* hexDump
 * utility hex dump function.
 * prints quadwords in hex separated by a dot;
 * if less than a quadword exists or it is not aligned.
 */
void hexDump(void *mem, u64 size);

void hexDump(void *mem, u64 size) {
	uint bytes = (uint) (size % sizeof(u64));
	u64 big = size / sizeof(u64);

	char buf[0x1000];
	string str = {
		.buf  = buf,
		.size = sizeof buf,
	};

	u64 i;
	for (i = 0; i < big; i++) {
		str = appendString(numAsString(str, ((u64*)mem)[i], 16), '.');
	}

	for (i = 0; i < bytes; i++) {
		str = numAsString(str, ((u8*)&((u64*)mem)[big])[i], 16);
	}

	writeString(str, 1);
}

static void print(char *str) {
	writeString(fromNullTermString(str), 1);
}

// start symbol
void _start(void);

void _start() {
	print("free mem:      ");
	hexDump(&_freeMemChunkHeap, sizeof(_freeMemChunkHeap));
	print("\nallocated mem: ");
	hexDump(&_allocatedMemChunkHeap, sizeof(_allocatedMemChunkHeap));
	print("\n");


	int *arr[10];

	int i;
	for (i = 0; i < 10; i++) {
		arr[i] = malloc(10 * 0x1000);
		hexDump(&arr[i], sizeof(int *)); print("\n");
	}

	print("free mem:      ");
	hexDump(&_freeMemChunkHeap, sizeof(_freeMemChunkHeap));
	print("\nallocated mem: ");
	hexDump(&_allocatedMemChunkHeap, sizeof(_allocatedMemChunkHeap));
	print("\n");

	for (i = 0; i < 10; i++) {
		free(arr[i]);
	}

	print("free mem:      ");
	hexDump(&_freeMemChunkHeap, sizeof(_freeMemChunkHeap));
	print("\nallocated mem: ");
	hexDump(&_allocatedMemChunkHeap, sizeof(_allocatedMemChunkHeap));
	print("\n");

	syscall(kSyscallExit, (u64[]){1, 0, 0, 0, 0, 0});
}
