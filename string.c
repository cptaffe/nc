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

const string emptyString = {.size = 0};

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
		str = appendString(str, digits[num % base]);
		num /= base;
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
		syscall(kSyscallWrite, (u64[]){fd, (u64) str.buf, str.len, 0, 0, 0});
	}
	return str;
}
