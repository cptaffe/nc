// Syscalls
// depends on def.h
// Defines SysCall function and related constants.

static int _syscall(int call, u64 p[const static 6]) {
     u64 ret;
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
	kSyscallRead    = 0,
	kSyscallWrite   = 1,
    kSyscallSocket  = 41,
    kSyscallConnect = 42,
    kSyscallFork    = 57,
	kSyscallExit    = 60
} SystemCall;

int syscall(SystemCall syscall, u64 argv[const static 6]) {
	return _syscall((int) syscall, argv);
}
