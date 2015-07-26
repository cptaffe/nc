// Memory utility methods.
// depends on def.h

// Mem contants
enum {
	pageSize = 0x1000
};

// Mem utility functions

/* upendMem
 * switches endiannes,
 * useful for going to/from network byte order
 * or for reversing strings.
 */
void upendMem(void *mem, size_t size);

/* zeroMem
 * simple zeroing method
 * uses quadword integer math to zero memory
 */
void zeroMem(void *mem, size_t size);


void upendMem(void *mem, size_t size) {
	if (size <= 0) return;
	u8 *mem8 = (u8*) mem;
	u64 i = 0, j = size-1;
	for (; i < j; i++, j--) {
		u8 b = mem8[i];
		mem8[i] = mem8[j];
		mem8[j] = b;
	}
}

// defined for linking, introduced by compiler.
void *memset(void *ptr, int value, size_t size) {
	u8 *mem = (u8*) ptr;
	size_t i;
	for (i = 0; i < size; i++) {
		mem[i] = (u8) value;
	}
	return ptr;
}

void *memcpy(void *dest, const void *src, size_t n) {
	const u8 *mem = (u8*) src;
	u8 *mem2 = (u8*) dest;
	size_t i;
	for (i = 0; i < n; i++) {
		mem2[i] = mem[i];
	}
	return dest;
}

void zeroMem(void *mem, size_t size) {
	memset(mem, 0, size);
}

/* MMap sycall wrapper code
 * Used by Mem to map pages of memory.
 */

enum {
	kMMapProtNone,
	kMMapProtRead   = 1 << 0,
	kMMapProtWrite  = 1 << 1,
	kMMapProtExec   = 1 << 2,
	kMMapProtAtomic = 1 << 4,

	kMMapFlagsShared  = 1 << 0,
	kMMapFlagsPrivate = 1 << 1,
	// Many flags omitted
	kMMapFlagsAnon    = 0x20
};

/* mapMem
 * wraps an mmap syscall
 * used to map memory pages
 */
void *mapMem(void *addr, u64 len, u64 prot, u64 flags,
	int fd, u64 offset);

/* unmapMem
 * wraps an munmap syscall
 * used to unmap memory pages
 */
int unmapMem(void *addr, u64 len);

void *mapMem(void *addr, u64 len, u64 prot, u64 flags,
	int fd, u64 offset) {
	return (void *) (uintptr_t) syscall(kSyscallMMap, (u64[6]){(u64) addr, len, prot, flags, (u64) fd, offset});
}

int unmapMem(void *addr, u64 len) {
	return (int) syscall(kSyscallMUnmap, (u64[6]){(u64) addr, len});
}

/* MemChunk
 * doubly linked list of memory chunks
 * kept as a header in each memory chunk allocated
 * and thusly does not require storing a pointer
 * to said memory.
 */
typedef struct _MemChunk {
	u64 size;
	struct _MemChunk *next;
	struct _MemChunk *prev;
} MemChunk;

/* MemChunkHeap
 * implements the Heapable interface
 * stores the beginning and end pointers to
 * the MemChunk doubly linked list for this heap.
 */
typedef struct {
	MemChunk *chunks;
	MemChunk *end;
	int len;
} MemChunkHeap;

/* free and allocated MemChunkHeap
 * global heaps for the storage of free
 * and allocated chunks, respectively.
 */
static MemChunkHeap _freeMemChunkHeap;
static MemChunkHeap _allocatedMemChunkHeap;

/* getMemChunk
 * follows linked list h->chunks n jumps,
 * returning the node it lands on.
 */
MemChunk *getMemChunk(MemChunkHeap *h, int n);

MemChunk *getMemChunk(MemChunkHeap *h, int n) {
	MemChunk *chunk = h->chunks;
	int i;
	for (i = 0; i < n; i++) {
		chunk = chunk->next;
	}
	return chunk;
}

/* MemChunkHeap implements Heapable
 * asHeapMemChunkHeap returns a Heap object
 * created from fulfilling the interface.
 */
void swapMemChunkHeap(void *v, int i, int j);
int lenMemChunkHeap(void *v);
bool lessMemChunkHeap(void *v, int i, int j);
void pushMemChunkHeap(void *v, void *x);
void *popMemChunkHeap(void *v);
Heap asHeapMemChunkHeap(MemChunkHeap *h);

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

/* MemChunkAddrHeap implements Heapable
 * asHeapMemChunkAddrHeap returns a Heap object
 * by piggybacking off MemChunkHeap but substituting
 * its own less method to order by memory address,
 * which is used for finding contiguous chunks.
 */
bool lessMemChunkAddrHeap(void *v, int i, int j);
Heap asHeapMemChunkAddrHeap(MemChunkHeap *h);

// Order heap by address
bool lessMemChunkAddrHeap(void *v, int i, int j) {
	MemChunkHeap *h = ((MemChunkHeap *) v);
	return getMemChunk(h, i) < getMemChunk(h, j);
}

Heap asHeapMemChunkAddrHeap(MemChunkHeap *h) {
	// Fill in Heap interface
	Heap oh = asHeapMemChunkHeap(h);
	oh.i.sort.less = lessMemChunkAddrHeap;
	return oh;
}

/* contignifyMemChunkHeap
 * finds contiguous chunks and combines them into larger
 * chunks. This function is routinely run on the
 * global free heap.
 */
void contignifyMemChunkHeap(Heap *fh);

// Combine contiguous chunks in free heap
void contignifyMemChunkHeap(Heap *fh) {
	MemChunkHeap chunk = {};
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
			if (((MemChunk *)&((u64*) last)[last->size / sizeof(u64)]) == m) {
				last->size += m->size;
			} else pushHeap(fh, last);
		}
		last = m;
	}
	pushHeap(fh, &last);
}

// Allocates n pages as a chunk
static MemChunk *_allocMemChunk(int pages) {
	void *mem = mapMem(nil, (u64) (pages * pageSize),
		kMMapProtRead | kMMapProtWrite,
		kMMapFlagsPrivate | kMMapFlagsAnon,
		-1, 0);
	mem = syscallError((i64) mem) ? nil : mem;
	if (mem == nil) return nil;
	MemChunk *m = (MemChunk *) mem;
	*m = (MemChunk) {
		.size = (u64) (pages * pageSize)
	};
	return m;
}

/* findMemChunk
 * takes a heap (ordered by minimum size)
 * and a minimum size, returns the smallest chunk larger
 * than the requested size, or nil.
 */
MemChunk *findMemChunk(Heap *fh, u64 size);

MemChunk *findMemChunk(Heap *fh, u64 size) {
	MemChunkHeap heap = {};
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

/* malloc and free
 * memory allocation and freeing methods.
 * all memory returned from malloc is zeroed.
 */
void free(void *mem);
void *malloc(size_t size);

void free(void *mem) {
	if (mem == nil) return;
	Heap fh = asHeapMemChunkHeap(&_freeMemChunkHeap);
	Heap ah = asHeapMemChunkHeap(&_allocatedMemChunkHeap);
	MemChunkHeap chunk = {};
	Heap lh = asHeapMemChunkHeap(&chunk);
	// Sift through heap to find appropriate address
	// Save chunks on local heap.
	MemChunk *m;
	while (lenMemChunkHeap(ah.heap) > 0) {
		m = (MemChunk *) popHeap(&ah);
		if (&((u8*)m)[-_headerSize()] == mem) {
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

void *malloc(size_t size) {
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
		int pages = (int) (size / pageSize);
		if (size % pageSize > 0) pages++;
		am = _allocMemChunk(pages);
		if (am == nil) return nil; // failed mmap
	}
	// Break off extra memory into another free chunk
	// only if there is enough extra space to allocate
	// another quadword aligned chunk with a header
	// chunk struct.
	if ((am->size - size) > (msize + sizeof(u64))) {
		// Store MemChunk header in upper extra memory.
		MemChunk *fm = (MemChunk*) &((u64*)am)[size / sizeof(u64)];
		*fm = (MemChunk) {
			.size = am->size - size
		};
		pushHeap(&fh, &fm);
		am->size = size;
	}
	pushHeap(&ah, am);
	// User only sees memory allocated for user
	// zero memory for security
	void *v = &((u8*) am)[msize];
	zeroMem(v, usize);
	return v;
}
