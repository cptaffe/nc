// Syscalls
// depends on def.h
// Defines SysCall function and related constants.

namespace syscall {

// System V ABI Section A.2.1
namespace {
i64 syscall(int call, u64 p0, u64 p1, u64 p2, u64 p3, u64 p4, u64 p5) {
	i64 ret;
	__asm__(
			// work around for lack of constraints for
			// r8, r9, and r10
			"movq %[p3], %%r10\n"
			"movq %[p4], %%r8\n"
			"movq %[p5], %%r9\n"

			"syscall\n"
			: "=a"(ret)
			: "a"(call), "D"(p0), "S"(p1), "d"(p2),
				[p3] "r"(p3), [p4] "r"(p4), [p5] "r"(p5)
			: "%r8", "%r9", "%r10",
				"%rcx", "%r11" // trashed by kernel
	 );
	 return ret;
}
} // namespace

// Syscall ids
enum class Call : int {
	kRead     = 0,
	kWrite    = 1,
	kMMap     = 9,
	kMProtect = 10,
	kMUnmap   = 11,
	kSocket   = 41,
	kConnect  = 42,
	kFork     = 57,
	kExit     = 60
};

i64 call(enum Call id, u64 p0, u64 p1, u64 p2, u64 p3, u64 p4, u64 p5) {
	return syscall(static_cast<int>(id), p0, p1, p2, p3, p4, p5);
}

// Returns errno value
int err(i64 n) {
	if (n > -0x1000 && n < 0) return static_cast<int>(-n);
	return 0;
}

} // namespace syscall
