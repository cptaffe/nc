
// Netcat utility
// Written with no standard library,
// for Linux only.
// Hopefully also a cleaner nc implementation

static int _syscall(int call, long long int p[const static 6]) {
     long long int ret;
     asm(
            // work around for lack of constraints for
            // r8, r9, and r10
            "movq %%r10, %5\n"
            "movq %%r8,  %6\n"
            "movq %%r9,  %7\n"

            "syscall\n"
            : "=a"(ret)
            : "a"(call), "D"(p[0]), "S"(p[1]), "d"(p[2]),
                     "r"(p[3]), "r"(p[4]), "r"(p[5])
			: "%r8", "%r9", "%r10"
     );
     return ret;
}

typedef enum {
	kSysCallRead   = 0,
	kSysCallWrite  = 1,
	kSysCallExit   = 60
} SystemCall;

static int SysCall(SystemCall syscall, long long int argv[const static 6]) {
	return _syscall((int) syscall, argv);
}

typedef int Socket;

typedef enum {
	// Linux protocols
	kAFLocal,
	kAFIPv4,
	kAFIPv6,
	kAFIPX,
	kAFNetlink,
	kAFX25,
	kAFAX25,
	kAFAtmPvc,
	kAFAppleTalk,
	kAFPacket,
	kAFAlg
} SocketDomain;

typedef enum {
	kSockStream    = 1,  // stream (connection) socket
	kSockDgram     = 2,  // datagram (conn.less) socket
	kSockRaw       = 3,  // raw socket
	kSockRDM       = 4,  // reliably-delivered message
	kSockSeqPacket = 5,  // sequential packet socket
	kSockDCCP      = 6,  // Datagram Congestion Control Protocol socket
	kSockPacket    = 10, // linux specific way of getting packets
	                     // at the dev level. For writing rarp and
	                     // other similar things on the user level.
	// TODO(cptaffe): add xorable flags
} SocketType;

// Assumes there is only one protocol for each, namely 0
Socket newSocket(SocketDomain domain, SocketType type) {
	// TODO(cptaffe): do stuff
	return -1;
}

int _start() {
	char buf[] = "hello\n";
	SysCall(kSysCallWrite, (long long int[]){1, (long long int) buf, sizeof(buf) - 1, 0, 0, 0});
	SysCall(kSysCallExit, (long long int[]){1, 0, 0, 0, 0, 0});
}
