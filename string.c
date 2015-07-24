// String type
// depends on def.h, syscall.c, mem.c
// Defines string type and related methods.

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

// Makes a new string from a character buffer of some size.
string *makeString(string *str, char buf[], u64 size) {
	// Sanity checks
	// NOTE: nil buffers are allowed if the size is 0.
	// nil strings are not allowed.
	if (str == nil || (buf == nil && size != 0)) return nil;

	zeroMem(str, sizeof(string));
	str->buf = buf;
	str->size = size;
}

// Sanity checks, allows every method to return nil on error.
static bool _stringOk(string *str) {
	return str != nil && str->len < str->size;
}

// Appends a character to a string if there is room.
string *pushString(string *str, char x) {
	if (!_stringOk(str)) return nil;
	if (str->len < str->size) {
		str->buf[str->len] = x;
		str->len++;
	}
	return str;
}

int popString(string *str) {
	if (!_stringOk(str)) return -1;
	str->len--;
	return str->buf[str->len];
}

string *appendString(string *str, char toAppend) {
	return pushString(str, toAppend);
}

inline u64 lenString(string *str) {
	return str->len;
}

string *subString(string *str, string *sub, u64 start, u64 end) {
	if (!_stringOk(str)) return nil;
	if (end > str->size) return nil;
	*sub = (string){
		.buf  = &str->buf[start],
		.size = str->size - start,
		.len  = end - start
	};
	return sub;
}

string *reverseString(string *str) {
	if (!_stringOk(str)) return nil;
	// Make use of endianness
	upendMem(str->buf, lenString(str));
	return str;
}

string *numAsString(string *str, u64 num, int base) {
	if (!_stringOk(str)) return nil;
	char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'a', 'b', 'c', 'd', 'e', 'f'};
	// Base out of range
	if (base < 2 || base > 16) return nil;

	u64 oldlen = lenString(str);

	while (num != 0 && str != nil) {
		str = appendString(str, digits[num % base]);
		num /= base;
	}

	// Reverse the substring written to.
	string substr;
	reverseString(subString(str, &substr, oldlen, lenString(str)));
	return str;
}

string *clearString(string *str) {
	if (!_stringOk(str)) return nil;
	str->len = 0;
	return str;
}

string *writeString(string *str, int fd) {
	if (!_stringOk(str)) return nil;
	if (str->len > 0) {
		syscall(kSyscallWrite, (u64[]){fd, (u64) str->buf, str->len, 0, 0, 0});
	}
}
