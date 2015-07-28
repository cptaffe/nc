// Memory utility methods.
// depends on def.h

// Mem utility functions
namespace mem {

// Mem contants
enum {
	pageSize = 0x1000
};

// upendMem
// Switches endiannes;
// useful for going to/from network byte order
// or for reversing strings.
// mem: memory chunk to swap endiannes
// size: size of mem
void upend(void *mem, size_t size) {
	if (size <= 0) return;
	u8 *mem8 = static_cast<u8*>(mem);
	u64 i = 0, j = size-1;
	for (; i < j; i++, j--) {
		u8 b = mem8[i];
		mem8[i] = mem8[j];
		mem8[j] = b;
	}
}

// zeroMem
// Simple zeroing method using memset.
// mem: memory chunk to zero
// size: size of mem
void zero(void *mem, size_t size) {
	memset(mem, 0, size);
}

namespace {
u64 align(u64 n, u64 alignment) {
	u64 r = n % alignment;
	if (r == 0) {
		return n;
	} else {
		return (n - r) + alignment;
	}
}
} // namespace

// MMap sycall wrapper code
// Used by Mem to map pages of memory.
class MMap {
public:
	enum class Prot : int {
		kNone,
		kRead   = 1 << 0,
		kWrite  = 1 << 1,
		kExec   = 1 << 2,
		kAtomic = 1 << 4
	};

	enum class Flag : int {
		kShared  = 1 << 0,
		kPrivate = 1 << 1,
		// Many flags omitted
		kAnon    = 0x20
	};

	// mapMem, unmapMem
	// Wraps mmap, munmap syscalls.
	// Used to map and unmap memory pages.
	// For more information see the respective man
	// pages for mmap and munmap.
	static void *map(void *addr, u64 len, u64 prot, u64 flags,
		int fd, u64 offset);
	static int unmap(void *addr, u64 len);
};

void *MMap::map(void *addr, u64 len, u64 prot, u64 flags,
	int fd, u64 offset) {
	return reinterpret_cast<void *>(syscall::call(syscall::Call::kMMap, reinterpret_cast<u64>(addr), len, prot, flags, static_cast<u64>(fd), offset));
}

int MMap::unmap(void *addr, u64 len) {
	return static_cast<int>(syscall::call(syscall::Call::kMUnmap, reinterpret_cast<u64>(addr), len, 0, 0, 0, 0));
}

// Chunk
// Doubly linked list of memory chunks;
// kept as a header in each memory chunk allocated
// and thusly does not require storing a pointer
// to said memory.
class Chunk {
public:
	Chunk(size_t s) : size_(s) {}

	// Allocate a new chunk
	static Chunk *map(int pages);
	static Chunk *malloc(Heap<Chunk *> *fh, Heap<Chunk *> *ah, size_t sz);
	// Free current chunk
	void free(Heap<Chunk *> *fh, Heap<Chunk *> *ah);

	// traverse
	// Follows linked list h->chunks n jumps,
	// returning the node it lands on.
	// chunk: Pointer to beginning of chunk list.
	// n: index in the list wanted.
	Chunk *traverse(int n);

	// Swap linked list location with other.
	void swap(Chunk *other);

	// Split chunk if enough room.
	Chunk *split(size_t sz);

	// Appends contiguous chunks in h until at least
	// size sz, else reverts and does not change size.
	// All appended memory is zeroed.
	void contignify(Heap<Chunk *> *h, size_t sz);

	// Converting from user address to chunk address.
	// User address is the address after the chunk section.
	static Chunk *atAddr(void *mem);
	void *addr();

	// returns the allocated size minus the header size.
	size_t size();

	// zero user memory.
	void zero();

	// returns needed size (with header, aligned)
	// from requested user size.
	static size_t neededSize(size_t sz);

	size_t size_ = 0; // total size, including header.
	Chunk *next = nullptr;
	Chunk *prev = nullptr;
private:
	// Size of chunk as header.
	static size_t header();
};

size_t Chunk::size() {
	return size_ - header();
}

Chunk *Chunk::traverse(int n) {
	Chunk *chunk = this;
	int i;
	for (i = 0; i < n && chunk != nullptr; i++) {
		chunk = chunk->next;
	}
	return chunk;
}

void Chunk::swap(Chunk *other) {
	if (this == other) return;
	Chunk *n = next;
	Chunk *p = prev;
	next = other->next;
	prev = other->prev;
	if (prev != nullptr) prev->next = this;
	if (next != nullptr) next->prev = this;
	other->next = n;
	other->prev = p;
	if (other->prev != nullptr) other->prev->next = other;
	if (other->next != nullptr) other->next->prev = other;
}

// Allocates n pages as a chunk
Chunk *Chunk::map(int pages) {
	void *mem = MMap::map(nullptr, static_cast<u64>(pages * pageSize),
		static_cast<int>(MMap::Prot::kRead) | static_cast<int>(MMap::Prot::kWrite),
		static_cast<int>(MMap::Flag::kPrivate) | static_cast<int>(MMap::Flag::kAnon),
		-1, 0);
	mem = syscall::err(reinterpret_cast<i64>(mem)) ? nullptr : mem;
	if (mem == nullptr) return nullptr;
	Chunk *m = static_cast<Chunk *>(mem);
	*m = Chunk(static_cast<u64>(pages * pageSize));
	return m;
}

size_t Chunk::header() {
	return align(sizeof(Chunk), sizeof(u64));
}

// Chunk::fromUserAddr
// gets chunk from returned user address.
Chunk *Chunk::atAddr(void *mem) {
	return reinterpret_cast<Chunk *>(reinterpret_cast<u8*>(mem)[-Chunk::header()]);
}

// Chunk::userAddr
// gets user memory address from chunk.
void *Chunk::addr() {
	return reinterpret_cast<void*>(reinterpret_cast<u8*>(this)[Chunk::header()]);
}

// splitChunk
// Splits chunk if large enough to be split,
// returns split chunk.
Chunk *Chunk::split(size_t sz) {
	// Break off extra memory into another free chunk
	// only if there is enough extra space to allocate
	// another quadword aligned chunk with a header
	// chunk struct.
	if ((size_ - sz) > (sz + sizeof(u64))) {
		// Store Chunk header in upper extra memory.
		Chunk *fm = reinterpret_cast<Chunk*>(&reinterpret_cast<u64*>(this)[sz / sizeof(u64)]);
		*fm = Chunk(size_ - sz);
		size_ = sz;
		return fm;
	}
	return nullptr;
}

// Zero memory after header block
void Chunk::zero() {
	u8 *mem = reinterpret_cast<u8*>(this);
	::mem::zero(&mem[Chunk::header()], size_ - Chunk::header());
}

// MemChunkHeap
// Implements the Heapable interface;
// stores the beginning and end pointers to
// the MemChunk doubly linked list for this heap.
// Implemented Heapable interface stores the chunks
// ordered by least size.
// chunks: Pointer to beginning of chunk linked list.
// end: Pointer to end of chunk linked list.
// len: Length of chunk linked list.
class ChunkHeap : public Heapable<Chunk *> {
	friend class ChunkAddrHeap;
public:
	void swap(int i, int j) override;
	int len() override;
	bool less(int i, int j) override;
	void push(Chunk *) override;
	Chunk *pop() override;
private:
	Chunk *chunks = nullptr;
	Chunk *end = nullptr;
	int _len = 0;
};

namespace {
// Global Memory Chunk Heaps
// Global heaps for the storage of free
// and allocated chunks.
// NOTE: The allocated heap is most likely
// unnecessary unless a gc is implemented.
ChunkHeap freeChunks;
ChunkHeap allocChunks;
} // namespace

void ChunkHeap::swap(int i, int j) {
	chunks->traverse(i)->swap(chunks->traverse(j));
}

int ChunkHeap::len() {
	return _len;
}

bool ChunkHeap::less(int i, int j) {
	return chunks->traverse(i)->size_ < chunks->traverse(j)->size_;
}

void ChunkHeap::push(Chunk *x) {
	// Link before push
	x->prev = end;
	x->next = nullptr;
	if (end == nullptr) {
		end = x;
		chunks = end; // setup head as well.
	} else {
		end->next = x;
		end = end->next;
	}
	_len++;
}

Chunk *ChunkHeap::pop() {
	Chunk *x = end;
	end = end->prev;
	end->next = nullptr;
	_len--;
	// Unlink after pop
	x->next = nullptr;
	x->prev = nullptr;
	return x;
}

// ChunkAddrHeap implements Heapable
// asHeapChunkAddrHeap returns a Heap object
// by piggybacking off ChunkHeap but substituting
// its own less method to order by memory address,
// which is used for finding contiguous chunks.
class ChunkAddrHeap : public ChunkHeap {
public:
	bool less(int i, int j) override;
};

// Order heap by address
bool ChunkAddrHeap::less(int i, int j) {
	return chunks->traverse(i) < chunks->traverse(j);
}

// Chunk::contignify
// finds free chunk contiguous (upwards) to chunk,
// combines them into one chunk which is returned.
// Continues recursively until chunk->size > size.
// TODO: Use ChunkAddrHeap to organize by address,
// then loop through once instead of n times.
void Chunk::contignify(Heap<Chunk *> *h, size_t sz) {
	ChunkHeap chunks;
	Heap<Chunk *> lh(&chunks);

	Chunk *m = nullptr;
	while (h->heap->len() > 0) {
		m = h->pop();
		// Checks if this chunk is contiguous
		if (reinterpret_cast<Chunk*>(&(reinterpret_cast<u64*>(this)[size_ / sizeof(u64)])) == m) {
			break; // found chunk
		} else lh.push(m);
	}

	// Push local heap back to h
	while (chunks.len() > 0) {
		h->push(lh.pop());
	}

	if (m != nullptr) {
		size_t oldsize = size_;
		size_ += m->size_;
		if (size_ < sz) {
			// try to find another chunk.
			contignify(h, sz);
			if (size_ < sz) {
				// reset old size.
				size_ = oldsize;
				return;
			}
		}
		// found enough memory
		::mem::zero(m, m->size_); // zero contiguous chunk
	}
}

size_t Chunk::neededSize(size_t sz) {
	return align(sz, sizeof(u64)) + Chunk::header();
}

namespace {
// contignify
// finds contiguous chunks and combines them into larger
// chunks. This function is routinely run on the
// global free heap.
// h: Heap to contignify, should be a ChunkHeap.
void contignify(Heap<Chunk *> *h) {
	ChunkAddrHeap chunk;
	Heap<Chunk *> lh(&chunk);

	// Heapify by memory address with ChunkAddrHeap
	while (h->heap->len() > 0) {
		lh.push(h->pop());
	}

	// Sift through looking for directly contiguous
	// chunks. Create larger chunks when found.
	Chunk *last = nullptr, *m;
	while (chunk.len() > 0) {
		m = lh.pop();
		if (last != nullptr) {
			// Organized by smallest address
			if (reinterpret_cast<Chunk*>(&(reinterpret_cast<u64*>(last)[last->size_ / sizeof(u64)])) == m) {
				last->size_ += m->size_;
			} else h->push(last);
		}
		last = m;
	}
	h->push(last);
}

// findChunk
// takes a heap (ordered by minimum size)
// and a minimum size, returns the smallest chunk larger
// than the requested size, or nullptr.
// h: Heap to search for eligable chunk in.
// size: Minimum size of chunk required.
Chunk *findChunk(Heap<Chunk*> *h, u64 size) {
	ChunkHeap heap;
	Heap<Chunk *> lh(&heap);
	Chunk *m = nullptr;
	// Sift through heap to find appropriate sized chunk
	// Save chunks on local heap.
	while (h->heap->len() > 0) {
		m = h->pop();
		if (m->size_ >= size) {
			break;
		}
		lh.push(m);
	}

	// Repush chunks from local heap
	while (lh.heap->len() > 0) {
		h->push(lh.pop());
	}

	// Have found chunk?
	if (m != nullptr && m->size_ >= size) return m;
	return nullptr;
}

// allocChunk
// Find an allocatable chunk in h or allocate a new
// chunk.
// h: heap to look for chunks in
// size: needed minimum chunk size
Chunk *allocChunk(Heap<Chunk *> *h, size_t size) {
	if (size == 0) return nullptr;
	// Calculate neede chunk size, including header space
	size = Chunk::neededSize(size);
	Chunk *m;
	if (!(m = findChunk(h, size))) {
		// If no chunks are large enough in h,
		// allocate enough pages for a new chunk.
		int pages = static_cast<int>(size / pageSize);
		if (size % pageSize > 0) pages++;
		m = Chunk::map(pages);

		// Split off extra memory and push it
		// back onto the heap.
		Chunk *split = m->split(size);
		h->push(split);
	}
	return m;
}
} // namespace

// malloc new chunk.
Chunk *Chunk::malloc(Heap<Chunk *> *fh, Heap<Chunk *> *ah, size_t sz) {
	// Find or allocate chunk of needed size.
	Chunk *am = allocChunk(fh, sz);
	if (am == nullptr) return nullptr;

	// Save chunk in allocated heap for book-keeping.
	ah->push(am);

	// User only sees memory allocated for user
	// zero memory for security
	am->zero();
	return am;
}

void Chunk::free(Heap<Chunk *> *fh, Heap<Chunk *> *ah) {
	ChunkHeap chunk;
	Heap<Chunk *> lh(&chunk);
	// Sift through heap to find appropriate address
	// Save chunks on local heap.
	Chunk *m;
	while (ah->heap->len() > 0) {
		m = ah->pop();
		if (this == m) {
			fh->push(m);
			break;
		}
		lh.push(m);
	}

	// Repush chunks from local heap
	while (lh.heap->len() > 0) {
		ah->push(lh.pop());
	}

	// attempt to contignify heap
	::mem::contignify(fh);
}

// malloc, realloc and free
// memory allocation and freeing methods.
// all memory returned from malloc is zeroed.
// For more information, see respective man pages.

void *malloc(size_t size) {
	Heap<Chunk *> fh(&freeChunks);
	Heap<Chunk *> ah(&allocChunks);
	Chunk *chunk = Chunk::malloc(&fh, &ah, size);
	if (chunk == nullptr) return nullptr;
	return chunk->addr();
}

void *realloc(void *mem, size_t size) {
	Heap<Chunk *> fh(&freeChunks);
	Heap<Chunk *> ah(&allocChunks);

	Chunk *chunk = Chunk::atAddr(mem);

	size = Chunk::neededSize(size);
	// Check if this chunk is large enough.
	if (chunk->size_ >= size) {
		return chunk->addr(); // memory is large enough.
	} else {
		chunk->contignify(&fh, size);
		if (chunk->size_ >= size) {
			return chunk->addr();
		}

		// Plan B: malloc, memcpy, free
		Chunk *nc = Chunk::malloc(&fh, &ah, size);
		memcpy(chunk->addr(), nc->addr(), nc->size());
		chunk->free(&fh, &ah);
		return nc->addr();
	}
}

void free(void *mem) {
	if (mem == nullptr) return;
	Heap<Chunk *> fh(&freeChunks);
	Heap<Chunk *> ah(&allocChunks);
	Chunk::atAddr(mem)->free(&fh, &ah);
}

} // namespace mem

// malloc, realloc, free
// placed in the global scope from the mem scope.
extern "C" void *malloc(size_t size) {
	return mem::malloc(size);
}

extern "C" void *realloc(void *mem, size_t size) {
	return mem::realloc(mem, size);
}

extern "C" void free(void *mem) {
	mem::free(mem);
}

// new, delete
// simply wrapping malloc, free.
// NOTE: memory allocated with realloc will delete
// with no problems.
void *operator new(size_t size) noexcept {
	return malloc(size);
}

void operator delete(void *mem) noexcept {
	free(mem);
}

void *operator new[](size_t size) noexcept {
	return operator new(size);
}

void operator delete[](void *mem) noexcept {
	operator delete(mem);
}
