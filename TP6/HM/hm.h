

#define INITIAL_HEAP_SIZE 1000
#define MMAP_THRESHOLD 10024
#define PAGE_SIZE 4096

struct bloc{
	int size_to_malloc;//Taille totale du bloc
	int available;//Place encore disponible dans le bloc
	int state;
	struct bloc *next;
	struct bloc *prev;
	void *donnee;//Pointeur vers la donnée du bloc
};

struct bloc_mmap{
	int size_usable;//Taille totale - taille(struct bloc)
	int state;
	struct bloc_mmap *next;
	struct bloc_mmap *prev;
	void *donnee;//Pointeur vers la donnée du bloc
};

void init_heap();
void *malloc(int size);
void free(void *ptr);
void *realloc(void *ptr, int size);
void *pmalloc(int size, bool protect);
