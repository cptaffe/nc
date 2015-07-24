// Memory utility methods.
// depends on def.h

// Switches endianness.
// To and from network byte order.
void upendMem(void *mem, u64 size) {
    if (size <= 0) return;
    u8 *mem8 = (u8*) mem;
    u64 i = 0, j = size-1;
    for (; i < j; i++, j--) {
        u8 b = mem8[i];
        mem8[i] = mem8[j];
        mem8[j] = b;
    }
}

// naive zeroing method
void zeroMem(void *mem, u64 size) {
    u8 *mem8 = (u8*) mem;
    u64 i;
    for (i = 0; i < size; i++) {
        mem8[i] = 0;
    }
}
