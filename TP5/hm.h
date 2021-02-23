

#define INITIAL_HEAP_SIZE 1000
#define MMAP_THRESHOLD 10024

struct bloc{
	int size_to_malloc;
	int state;
	struct bloc *next;
};


void init_heap();
void *__malloc(int size);
void __free(void *ptr);
void *__realloc(void *ptr, int size);
