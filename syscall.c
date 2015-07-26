// Syscalls
// depends on def.h
// Defines SysCall function and related constants.

// System V ABI Section A.2.1
static u64 _syscall(int call, u64 p[const static 6]) {
	 u64 ret;
	 asm(
			// work around for lack of constraints for
			// r8, r9, and r10
			"movq %[p3], %%r10\n"
			"movq %[p4], %%r8\n"
			"movq %[p5], %%r9\n"

			"syscall\n"
			: "=a"(ret)
			: "a"(call), "D"(p[0]), "S"(p[1]), "d"(p[2]),
				[p3] "r"(p[3]), [p4] "r"(p[4]), [p5] "r"(p[5])
			: "%r8", "%r9", "%r10",
				"%rcx", "%r11" // trashed by kernel
	 );
	 return ret;
}

typedef enum {
	kSyscallRead     = 0,
	kSyscallWrite    = 1,
	kSyscallMMap     = 9,
	kSyscallMProtect = 10,
	kSyscallMUnmap   = 11,
	kSyscallSocket   = 41,
	kSyscallConnect  = 42,
	kSyscallFork     = 57,
	kSyscallExit     = 60
} SystemCall;

// Returns errno value
int syscallError(i64 n) {
	if (n > -0x1000 && n < 0) return -n;
	return 0;
}

u64 syscall(SystemCall syscall, u64 argv[const static 6]) {
	return _syscall((int) syscall, argv);
}
