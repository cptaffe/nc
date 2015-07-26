// Heap interface,
// Inspired by the Go Heap.

// Sortable Interface
typedef struct {
	int (*len)(void *sortable);
	bool (*less)(void *sortable, int i, int j);
	void (*swap)(void *sortable, int i, int j);
} Sortable;

// Heapable Interface
typedef struct {
	Sortable sort;
	void *(*pop)(void *heapable);
	void (*push)(void *heapable, void *x);
} Heapable;

// Heap type
typedef struct {
	void *heap;
	Heapable i;
} Heap;

// Heap methods
void initHeap(Heap *h);
void pushHeap(Heap *h, void *x);
void *popHeap(Heap *h);
void *removeHeap(Heap *h, int i);
void fixHeap(Heap *h, int i);

static void _upHeap(Heap *h, int j) {
	for (;;) {
		int i = (j - 1) / 2; // parent
		if (i == j || !h->i.sort.less(h->heap, j, i)) {
			break;
		}
		h->i.sort.swap(h->heap, i, j);
		j = i;
	}
}

static void _downHeap(Heap *h, int i, int n) {
	for (;;) {
		int j1 = 2*i + 1;
		if (j1 >= n || j1 < 0) { // j1 < 0 after int overflow
			break;
		}
		int j = j1; // left child
		int j2 = j1 + 1;
		if (j2 < n && !h->i.sort.less(h->heap, j1, j2)) {
			j = j2; // right child
		}
		if (!h->i.sort.less(h->heap, j, i)) {
			break;
		}
		h->i.sort.swap(h->heap, i, j);
		i = j;
	}
}

void initHeap(Heap *h) {
	// heapify
	int n = h->i.sort.len(h->heap);
	int i;
	for (i = n/2 - 1; i > 0; i--) {
		_downHeap(h, i, n);
	}
}

void pushHeap(Heap *h, void *x) {
	h->i.push(h->heap, x);
	_upHeap(h, h->i.sort.len(h->heap) - 1);
}

void *popHeap(Heap *h) {
	int n = h->i.sort.len(h->heap) - 1;
	h->i.sort.swap(h->heap, 0, n);
	_downHeap(h, 0, n);
	return h->i.pop(h->heap);
}

void *removeHeap(Heap *h, int i) {
	int n = h->i.sort.len(h->heap) - 1;
	if (n != i) {
		h->i.sort.swap(h->heap, i, n);
		_downHeap(h, i, n);
		_upHeap(h, i);
	}
	return h->i.pop(h->heap);
}

void fixHeap(Heap *h, int i) {
	_downHeap(h, i, h->i.sort.len(h->heap));
	_upHeap(h, i);
}
