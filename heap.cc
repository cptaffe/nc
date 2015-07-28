// Heap interface,
// Inspired by the Go Heap.

// Sortable Interface
class Sortable {
public:
	virtual ~Sortable() {}
	virtual int len() = 0;
	virtual bool less(int i, int j) = 0;
	virtual void swap(int i, int j) = 0;
};

// Heapable Interface
template <typename T>
class Heapable : public Sortable {
public:
	virtual ~Heapable() {}
	virtual T pop() = 0;
	virtual void push(T x) = 0;
};

template <typename T>
class Heap {
public:
	Heap(Heapable<T> *h) : heap(h) {}
	void init();
	void push(T x);
	T pop();
	T remove(int i);
	void fix(int i);
	Heapable<T> *heap;
private:
	void up(int j);
	void down(int i, int n);
};

template <typename T>
void Heap<T>::up(int j) {
	for (;;) {
		int i = (j - 1) / 2; // parent
		if (i == j || !heap->less(j, i)) {
			break;
		}
		heap->swap(i, j);
		j = i;
	}
}

template <typename T>
void Heap<T>::down(int i, int n) {
	for (;;) {
		int j1 = 2*i + 1;
		if (j1 >= n || j1 < 0) { // j1 < 0 after int overflow
			break;
		}
		int j = j1; // left child
		int j2 = j1 + 1;
		if (j2 < n && !heap->less(j1, j2)) {
			j = j2; // right child
		}
		if (!heap->less(j, i)) {
			break;
		}
		heap->swap(i, j);
		i = j;
	}
}

template <typename T>
void Heap<T>::init() {
	// heapify
	int n = heap->len();
	int i;
	for (i = n/2 - 1; i > 0; i--) {
		down(i, n);
	}
}

template <typename T>
void Heap<T>::push(T x) {
	heap->push(x);
	up(heap->len() - 1);
}

template <typename T>
T Heap<T>::pop() {
	int n = heap->len() - 1;
	heap->swap(0, n);
	down(0, n);
	return heap->pop();
}

template <typename T>
T Heap<T>::remove(int i) {
	int n = heap->len(heap) - 1;
	if (n != i) {
		heap->swap(i, n);
		down(i, n);
		up(i);
	}
	return heap->pop();
}

template <typename T>
void Heap<T>::fix(int i) {
	down(i, heap->len());
	up(i);
}
