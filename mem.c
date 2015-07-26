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

typedef struct _MemChunk {
	void *mem;
	u64 size;
	struct _MemChunk *next;
	struct _MemChunk *prev;
} MemChunk;

typedef struct {
	MemChunk *chunks;
	MemChunk *end;
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

MemChunk *getMemChunk(MemChunkHeap *h, int n) {
	MemChunk *chunk = h->chunks;
	int i;
	for (i = 0; i < n; i++) {
		chunk = chunk->next;
	}
	return chunk;
}

void swapMemChunkHeap(void *v, int i, int j) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	MemChunk *a = getMemChunk(h, i);
	MemChunk *b = getMemChunk(h, j);
	if (a == b) return;
	MemChunk *n = b->next;
	MemChunk *p = b->prev;
	b->next = a->next;
	b->prev = a->prev;
	if (b->prev != nil) b->prev->next = b;
	if (b->next != nil) b->next->prev = b;
	a->next = n;
	a->prev = p;
	if (a->prev != nil) a->prev->next = a;
	if (a->next != nil) a->next->prev = a;
}

int lenMemChunkHeap(void *v) {
	return ((MemChunkHeap *) v)->len;
}

bool lessMemChunkHeap(void *v, int i, int j) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	return getMemChunk(h, i)->size < getMemChunk(h, j)->size;
}

void pushMemChunkHeap(void *v, void *x) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	MemChunk *y = (MemChunk *) x;
	y->prev = h->end;
	y->next = nil;
	if (h->end == nil) {
		h->end = y;
		h->chunks = h->end; // setup head as well.
	} else {
		h->end->next = y;
		h->end = h->end->next;
	}
	h->len++;
}

void *popMemChunkHeap(void *v) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	MemChunk *x = h->end;
	x->next = x->prev = nil;
	h->end->prev->next = nil;
	h->end = h->end->prev;
	h->len--;
	return x;
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
	return getMemChunk(h, i)->mem < getMemChunk(h, j)->mem;
}

Heap asHeapMemChunkAddrHeap(MemChunkHeap *h) {
	// Fill in Heap interface
	Heap oh = asHeapMemChunkHeap(h);
	oh.i.sort.less = lessMemChunkAddrHeap;
	return oh;
}

MemChunk *allocMemChunk(int pages) {
	void *mem = mapMem(nil, pages * pageSize,
		kMMapProtRead | kMMapProtWrite,
		kMMapFlagsPrivate | kMMapFlagsAnon,
		-1, 0);
	mem = syscallError((u64) mem) ? nil : mem;
	if (mem == nil) return nil;
	MemChunk *m = (MemChunk *) mem;
	*m = (MemChunk) {
		.mem  = mem,
		.size = pages * pageSize
	};
	return m;
}

MemChunk *findMemChunk(Heap *fh, u64 size) {
	MemChunkHeap heap = {0};
	Heap lh = asHeapMemChunkHeap(&heap);
	MemChunk *m = nil;
	// Sift through heap to find appropriate sized chunk
	// Save chunks on local heap.
	while (lenMemChunkHeap(fh->heap) > 0) {
		m = (MemChunk *) popHeap(fh);
		if (m->size >= size) {
			break;
		}
		pushHeap(&lh, m);
	}

	// Repush chunks from local heap
	while (lenMemChunkHeap(lh.heap) > 0) {
		pushHeap(fh, popHeap(&lh));
	}

	// Have found chunk?
	if (m != nil && m->size >= size) return m;
	return nil;
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
	MemChunk *last = nil, *m;
	while (lenMemChunkHeap(lh.heap) > 0) {
		m = (MemChunk*) popHeap(&lh);
		if (last != nil) {
			// Organized by smallest address
			if (&((u8*) last->mem)[last->size] == m->mem) {
				m = (MemChunk *) last->mem;
				*m = (MemChunk) {
					.mem  = m,
					.size = last->size + m->size
				};
			} else pushHeap(fh, last);
		}
		last = m;
	}
	pushHeap(fh, &last);
}

static u64 _align(u64 n, u64 alignment) {
	u64 r = n % alignment;
	if (r == 0) {
		return n;
	} else {
		return (n - r) + alignment;
	}
}

static u64 _headerSize() {
	return _align(sizeof(MemChunk), sizeof(u64));
}

void free(void *mem) {
	if (mem == nil) return;
	Heap fh = asHeapMemChunkHeap(&_freeMemChunkHeap);
	Heap ah = asHeapMemChunkHeap(&_allocatedMemChunkHeap);
	MemChunkHeap chunk = {0};
	Heap lh = asHeapMemChunkHeap(&chunk);
	// Sift through heap to find appropriate address
	// Save chunks on local heap.
	MemChunk *m;
	while (lenMemChunkHeap(ah.heap) > 0) {
		m = (MemChunk *) popHeap(&ah);
		if (&((u8*)m->mem)[-_headerSize()] == mem) {
			pushHeap(&fh, m);
			break;
		}
		pushHeap(&lh, m);
	}

	// Repush chunks from local heap
	while (lenMemChunkHeap(lh.heap) > 0) {
		pushHeap(&ah, popHeap(&lh));
	}

	// attempt to contignify heap
	contignifyMemChunkHeap(&fh);
}

void *malloc(u32 size) {
	if (size == 0) return nil;
	// Keep quadword alignment
	u64 usize = _align(size, sizeof(u64));
	u64 msize = _headerSize();
	size = usize + msize;
	Heap fh = asHeapMemChunkHeap(&_freeMemChunkHeap);
	Heap ah = asHeapMemChunkHeap(&_allocatedMemChunkHeap);
	MemChunk *am;
	if (!(am = findMemChunk(&fh, size))) {
		// Allocate enough pages in one chunk
		int pages = size / pageSize;
		if (size % pageSize > 0) pages++;
		am = allocMemChunk(pages);
		if (am->mem == nil) return nil; // failed mmap
	}
	// Break off extra memory into another free chunk
	// only if there is enough extra space to allocate
	// another quadword aligned chunk with a header
	// chunk struct.
	if ((am->size - size) > (msize + sizeof(u64))) {
		// Store MemChunk header in upper extra memory.
		MemChunk *fm = (MemChunk*) &((u8*)am->mem)[size];
		*fm = (MemChunk) {
			.mem  = fm,
			.size = am->size - size
		};
		pushHeap(&fh, &fm);
		am->size = size;
	}
	pushHeap(&ah, am);
	// User only sees memory allocated for user
	// zero memory for security
	void *v = &((u8*) am->mem)[msize];
	zeroMem(v, usize);
	return v;
}
