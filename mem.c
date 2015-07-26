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

typedef struct {
	void *mem;
	u64 size;
} MemChunk;

typedef struct {
	MemChunk chunks[100];
	int len;
	int pages; // number of allocated pages
} MemChunkHeap;

MemChunkHeap _freeMemChunkHeap;
MemChunkHeap _allocatedMemChunkHeap;

typedef enum {
	kMMapProtNone,
	kMMapProtRead   = 1 << 0,
	kMMapProtWrite  = 1 << 1,
	kMMapProtExec   = 1 << 2,
	kMMapProtAtomic = 1 << 4,
} MMapProt;

typedef enum {
	kMMapFlagsShared  = 1 << 0,
	kMMapFlagsPrivate = 1 << 1,
	// Many flags omitted
	kMMapFlagsAnon    = 0x20
} MMapFlags;

enum { pageSize = 0x1000 };

void *mapMem(void *addr, u64 len, MMapProt prot, MMapFlags flags,
	int fd, u64 offset) {
	return (void *) syscall(kSyscallMMap, (u64[6]){(u64) addr, len, prot, flags, fd, offset});
}

int unmapMem(void *addr, u64 len) {
	return (int) syscall(kSyscallMUnmap, (u64[6]){(u64) addr, len});
}

void swapMemChunkHeap(void *v, int i, int j) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	MemChunk b = h->chunks[i];
	h->chunks[i] = h->chunks[j];
	h->chunks[j] = b;
}

int lenMemChunkHeap(void *v) {
	return ((MemChunkHeap *) v)->len;
}

bool lessMemChunkHeap(void *v, int i, int j) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	return h->chunks[i].size < h->chunks[j].size;
}

void pushMemChunkHeap(void *v, void *x) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	h->chunks[h->len] = *((MemChunk *) x);
	h->len++;
}

void *popMemChunkHeap(void *v) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	h->len--;
	return &h->chunks[h->len];
}

Heap asHeapMemChunkHeap(MemChunkHeap *h) {
	// Fill in Heap interface
	return (Heap){
		.heap = h,
		.i = (Heapable){
			.sort = (Sortable){
				.len  = lenMemChunkHeap,
				.less = lessMemChunkHeap,
				.swap = swapMemChunkHeap
			},
			.pop  = popMemChunkHeap,
			.push = pushMemChunkHeap
		}
	};
}

// Order heap by address
bool lessMemChunkAddrHeap(void *v, int i, int j) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	return h->chunks[i].mem < h->chunks[j].mem;
}

Heap asHeapMemChunkAddrHeap(MemChunkHeap *h) {
	// Fill in Heap interface
	return (Heap){
		.heap = h,
		.i = (Heapable){
			.sort = (Sortable){
				.len  = lenMemChunkHeap,
				.less = lessMemChunkAddrHeap,
				.swap = swapMemChunkHeap
			},
			.pop  = popMemChunkHeap,
			.push = pushMemChunkHeap
		}
	};
}

MemChunk allocMemChunk(int pages) {
	void *mem = mapMem(nil, pages * pageSize,
		kMMapProtRead | kMMapProtWrite,
		kMMapFlagsPrivate | kMMapFlagsAnon,
		-1, 0);
	MemChunk m = {
		.mem = syscallError((u64) mem) ? nil : mem,
		.size = pages * pageSize
	};
	return m;
}

MemChunk deallocMemChunk(MemChunk chunk) {
	// not a page aligned address
	if (((u64) chunk.mem) != ((u64) chunk.mem) % pageSize) return chunk;
	// largest number of pages that can be unmapped
	int pages = chunk.size / pageSize;
	if (pages == 0) return chunk; // not large enough
	unmapMem(chunk.mem, pages * pageSize);
	return (MemChunk) {
		.mem  = &((u8*) chunk.mem)[pages * pageSize],
		.size = chunk.size - (pages * pageSize)
	};
}

bool findMemChunk(Heap *fh, MemChunk *m, u64 size) {
	MemChunkHeap chunk = {0};
	Heap lh = asHeapMemChunkHeap(&chunk);
	// Sift through heap to find appropriate sized chunk
	// Save chunks on local heap.
	while (lenMemChunkHeap(fh->heap) > 0) {
		*m = *((MemChunk *) popHeap(fh));
		if (m->size >= size) {
			break;
		}
		pushHeap(&lh, &m);
	}

	// Repush chunks from local heap
	while (lenMemChunkHeap(lh.heap) > 0) {
		pushHeap(fh, popHeap(&lh));
	}

	// Have found chunk?
	if (m->size >= size) return true;
	return false;
}

// Combine contiguous chunks in free heap
void contignifyMemChunkHeap(Heap *fh) {
	MemChunkHeap chunk = {0};
	Heap lh = asHeapMemChunkAddrHeap(&chunk);

	// Heapify by memory address with MemChunkAddrHeap
	while (lenMemChunkHeap(fh->heap) > 0) {
		pushHeap(&lh, popHeap(fh));
	}

	// Sift through looking for directly contiguous
	// chunks. Create larger chunks when found.
	MemChunk last = {0}, m = {0};
	while (lenMemChunkHeap(lh.heap) > 0) {
		m = *((MemChunk*) popHeap(&lh));
		if (last.size > 0) {
			// Organized by smallest address
			if (&((u8*) last.mem)[last.size] == m.mem) {
				m = (MemChunk) {
					.mem  = last.mem,
					.size = last.size + m.size
				};
			} else pushHeap(fh, &last);
		}
		last = m;
	}
	pushHeap(fh, &last);
}

void free(void *mem) {
	if (mem == nil) return;
	Heap fh = asHeapMemChunkHeap(&_freeMemChunkHeap);
	Heap ah = asHeapMemChunkHeap(&_allocatedMemChunkHeap);
	MemChunkHeap chunk = {0};
	Heap lh = asHeapMemChunkHeap(&chunk);
	// Sift through heap to find appropriate address
	// Save chunks on local heap.
	MemChunk m = {0};
	while (lenMemChunkHeap(ah.heap) > 0) {
		m = *((MemChunk *) popHeap(&ah));
		if (m.mem == mem) {
			pushHeap(&fh, &m);
			break;
		}
		pushHeap(&lh, &m);
	}

	// Repush chunks from local heap
	while (lenMemChunkHeap(lh.heap) > 0) {
		pushHeap(&ah, popHeap(&lh));
	}

	// attempt to contignify heap
	contignifyMemChunkHeap(&fh);
}

static u64 _align(u64 n, u64 alignment) {
	return (n - (n % alignment)) + ((n % alignment) ? alignment : 0);
}

void *malloc(u64 size) {
	if (size == 0) return nil;
	// Keep quadword alignment
	size = _align(size, sizeof(u64));
	Heap fh = asHeapMemChunkHeap(&_freeMemChunkHeap);
	Heap ah = asHeapMemChunkHeap(&_allocatedMemChunkHeap);
	MemChunk m = {0};
	if (!findMemChunk(&fh, &m, size)) {
		// Allocate enough pages in one chunk
		int pages = size / pageSize;
		if (size % pageSize > 0) pages++;
		m = allocMemChunk(pages);
		if (m.mem == nil) return nil; // failed mmap
	}
	// Break off extra memory into another free chunk
	if (m.size - size > 0) {
		MemChunk fm = {
			.mem  = &((u8*)m.mem)[size],
			.size = m.size - size
		};
		pushHeap(&fh, &fm);
		m.size = size;
	}
	pushHeap(&ah, &m);
	return m.mem;
}
