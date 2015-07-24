
#include "def.h"
// Syscall, Mem depends on def.h
#include "syscall.c"
#include "mem.c"
// String depends on def.h, syscall.c, mem.c
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
	writeString(appendString(numAsString(&(string){
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
		appendString(numAsString(&str, ((u64*)mem)[i], 16), '.');
	}

	for (i = 0; i < bytes; i++) {
		numAsString(&str, ((u8*)&((u64*)mem)[big])[i], 16);
	}

	writeString(appendString(&str, '\n'), 1);
}

void _start() {
	Sock sock;
	SockAddrv4 sockAddr;
	makeSock(&sock, kSockDomainIPv4, kSockTypeStream);
	makeSockAddrv4(&sockAddr, 16);
	hexDump(&sockAddr, sizeof(sockAddr));
	syscall(kSyscallExit, (u64[]){1, 0, 0, 0, 0, 0});
}
