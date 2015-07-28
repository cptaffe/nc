// Simple declarations

// nullptr c++ definition
typedef decltype(nullptr) nullptr_t;

typedef unsigned int uint;
typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed long int i32;
typedef unsigned long int u32;
typedef signed long long int i64;
typedef unsigned long long int u64;

// get around u64 -> void* warnings
typedef u64 uintptr_t;
// library defs use 32 bits for size.
typedef u32 size_t;
