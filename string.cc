// String type
// depends on def.h, syscall.c, mem.c
// Defines string type and related methods.

// TODO: remove mem linkage shim
void upendMem(void *mem, size_t size);

enum {
	kStringFdIn,
	kStringFdOut,
	kStringFdErr,
};

typedef struct {
	char *buf;
	u64 size;
	u64 len;
} string;

// emptyString, empty string constant
extern const string emptyString;

const string emptyString = {.size = 0};

// string utility methods
bool isEmptyString(string str);
string newString(char buf[], u64 size);
string pushString(string str, char x);
int popString(string str);
string appendString(string str, char toAppend);
string subString(string str, u64 start, u64 end);
string reverseString(string str);
string numAsString(string str, u64 num, int base);
string clearString(string str);
string writeString(string str, int fd);
string fromNullTermString(char *str);

/* hexDump, hexDumps
 * Utility hex dump function.
 * prints quadwords in hex separated by a dot;
 * if less than a quadword exists or it is not aligned.
 */
void hexDump(void *mem, size_t size);
void hexDumps(char *prefix, void *mem, size_t size);

void hexDumps(char *prefix, void *mem, size_t size) {
	writeString(fromNullTermString(prefix), 1);
	hexDump(mem, size);
	writeString(fromNullTermString("\n"), 1);
}

void hexDump(void *mem, size_t size) {
	uint bytes = (uint) (size % sizeof(u64));
	size_t big = size / sizeof(u64);

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

bool isEmptyString(string str) {
	return str.size == 0;
}

// Makes a new string from a character buffer of some size.
string newString(char buf[], u64 size) {
	return (string) {
		.buf  = buf,
		.size = size
	};
}

// Sanity checks, allows every method to return nil on error.
static bool _stringOk(string str) {
	return str.len < str.size; // implies !isEmptyString
}

// Appends a character to a string if there is room.
string pushString(string str, char x) {
	if (!_stringOk(str)) return emptyString;
	if (str.len < str.size) {
		str.buf[str.len] = x;
		str.len++;
	}
	return str;
}

int popString(string str) {
	if (!_stringOk(str)) return -1;
	str.len--;
	return str.buf[str.len];
}

string appendString(string str, char toAppend) {
	return pushString(str, toAppend);
}

string subString(string str, u64 start, u64 end) {
	if (!_stringOk(str) || end > str.size) return emptyString;
	return (string){
		.buf  = &str.buf[start],
		.size = str.size - start,
		.len  = end - start
	};
}

string reverseString(string str) {
	if (!_stringOk(str)) return emptyString;
	// Make use of endianness
	upendMem(str.buf, str.len);
	return str;
}

string numAsString(string str, u64 num, int base) {
	if (!_stringOk(str)) return emptyString;
	char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'a', 'b', 'c', 'd', 'e', 'f'};
	// Base out of range
	if (base < 2 || base > 16) return emptyString;

	u64 oldlen = str.len;

	while (num != 0 && !isEmptyString(str)) {
		str = appendString(str, digits[num % (u64) base]);
		num /= (u64) base;
	}

	// Reverse the substring written to.
	reverseString(subString(str, oldlen, str.len));
	return str;
}

string clearString(string str) {
	if (!_stringOk(str)) return emptyString;
	str.len = 0;
	return str;
}

string writeString(string str, int fd) {
	if (!_stringOk(str)) return emptyString;
	if (str.len > 0) {
		syscall(kSyscallWrite, (u64[]){(u64) fd, (u64) str.buf, str.len, 0, 0, 0});
	}
	return str;
}

string fromNullTermString(char *str) {
	u64 i;
	for (i = 0; str[i] != '\0'; i++) {}
	return (string){
		.buf  = str,
		.size = i + 1,
		.len  = i
	};
}
