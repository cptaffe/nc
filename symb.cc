// Compiler generated sysmbols

namespace {

class atExitFunc {
public:
	atExitFunc() {}
	atExitFunc(void (*d)(void *), void *a, void *s)
		: destructor(d), arg(a), dso(s) {}
	void call() { destructor(arg); }
	bool match(void *d) { return dso == d; }
private:
	void (*destructor)(void *) = nullptr;
	void *arg;
	void *dso;
};

enum { kMaxFuncs = 0x80 };

int len = 0;
atExitFunc exitFuncs[kMaxFuncs];

} // namespace

extern "C" {
// memset
// Sets memory using simple integer math.
// NOTE: defined for linking, introduced by compiler.
void *memset(void *ptr, int value, size_t size) {

	uint bytes = static_cast<uint>(size % sizeof(u64));
	u64 big = size / sizeof(u64);

	u64 *mem64 = static_cast<u64*>(ptr);
	u64 swath = 0;
	size_t i;
	for (i = 0; i < sizeof(u64); i++) {
		reinterpret_cast<u8*>(&swath)[i] = static_cast<u8>(value);
	}

	for (i = 0; i < big; i++) {
		mem64[i] = swath;
	}

	u8 *mem8 = reinterpret_cast<u8*>(&mem64[big]);
	for (i = 0; i < bytes; i++) {
		mem8[i] = static_cast<u8>(value);
	}
	return ptr;
}

// memcpy
// NOTE: defined for linking, introduced by compiler.
void *memcpy(void *dest, const void *src, size_t n) {
	const u8 *mem = static_cast<const u8*>(src);
	u8 *mem2 = static_cast<u8*>(dest);
	size_t i;
	for (i = 0; i < n; i++) {
		mem2[i] = mem[i];
	}
	return dest;
}



// __dso_handle
// NOTE: C++ runtime. Defined for linking, introduced by compiler.
void *__dso_handle = nullptr;

// __cxa_pure_virtual
// NOTE: C++ runtime. Defined for linking, introduced by compiler.
void __cxa_pure_virtual() {
	// pure virtual function call cannot be made.
	char error[] = "Unimplemented virtual function called\n";
	syscall::call(syscall::Call::kWrite, reinterpret_cast<u64>(error), sizeof(error)-1, 0, 0, 0, 0);
}

// __cxa_atexit
// NOTE: C++ runtime. Defined for linking, introduced by compiler.
int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso) {
	if (len < kMaxFuncs) {
		exitFuncs[len++] = atExitFunc(destructor, arg, dso);
		return 0;
	}
	return -1; // failure
}

// __cxa_finalize
// NOTE: C++ runtime. Defined for linking, introduced by compiler.
void __cxa_finalize(void *f) {
	if (f == nullptr) {
		// Must be called in reverse order.
		for (int i = len; i >= 0; i--) {
			exitFuncs[i].call();
		}
		len = 0;
		return;
	}

	for (int i = len; i >= 0; i--) {
		if (exitFuncs[i].match(f)) exitFuncs[i].call();
		memcpy(&exitFuncs[i], &exitFuncs[i + 1], static_cast<size_t>(len - i) * sizeof(atExitFunc));
		len--;
	}
}
} // extern "C"
