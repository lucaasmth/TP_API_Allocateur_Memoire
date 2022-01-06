/* On inclut l'interface publique */
#include "mem.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <stdio.h>

/* Définition de l'alignement recherché
 * Avec gcc, on peut utiliser __BIGGEST_ALIGNMENT__
 * sinon, on utilise 16 qui conviendra aux plateformes qu'on cible
 */
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Elle peut bien évidemment être complétée
*/
struct allocator_header {
    size_t memory_size;
	mem_fit_function_t *fit;
	struct fb *first;
};

/* La seule variable globale autorisée
 * On trouve à cette adresse le début de la zone à gérer
 * (et une structure 'struct allocator_header)
 */
static void* memory_addr;

static inline void *get_system_memory_addr() {
	return memory_addr;
}

static inline struct allocator_header *get_header() {
	struct allocator_header *h;
	h = get_system_memory_addr();
	return h;
}

static inline size_t get_system_memory_size() {
	return get_header()->memory_size;
}


struct fb {
	size_t size;
	struct fb* next;
	int free;
};


void mem_init(void* mem, size_t taille)
{
	memory_addr = mem;
	*(size_t*)memory_addr = taille;

	get_header()->memory_size = taille;
	get_header()->first = mem + sizeof(struct allocator_header);
	get_header()->first->size = taille - sizeof(struct allocator_header) - sizeof(struct fb);
	get_header()->first->next = NULL;
	get_header()->first->free = 1;

	assert(mem == get_system_memory_addr());
	assert(taille == get_system_memory_size());

	//On défini la fonction de recherche à mem_fit_first par défaut
	mem_fit(&mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {
	struct fb* bloc = get_system_memory_addr() + sizeof(struct allocator_header);
	printf("Fin de la mémoire: %ld\n", get_system_memory_size());
	printf("Première zone libre: %ld\n", (long int)get_header()->first - (long int)get_system_memory_addr());
	while (bloc < (struct fb*)(get_system_memory_addr() + get_system_memory_size())) {
		print(bloc, bloc->size, bloc->free);
		bloc = (struct fb*)((void*)bloc + bloc->size + sizeof(struct fb));
	}
}

void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

void* mem_alloc(size_t taille) {
	struct fb* fb = get_header()->fit(get_header()->first, taille);
	if(fb == NULL) return NULL;
	if(taille != fb->size){
		struct fb* newfb = (void*)(fb) + taille + sizeof(struct fb);
		if(get_header()->first == fb) {
			newfb->next = get_header()->first;
			get_header()->first = newfb;
		} 
		else {
			struct fb* prev = get_header()->first;
			while(prev != 0) {
				if(prev->next == fb) break;
				prev = prev->next;
			}
			prev->next = newfb;
		}
		newfb->size = fb->size - taille - sizeof(struct fb);
		newfb->free = 1;
	} else {
		if(get_header()->first == fb) {
			get_header()->first = get_header()->first->next;
		}
	}
	fb->size = taille;
	fb->free = 0;
	return fb;
}


void mem_free(void* mem) {
	struct fb* bloc_to_free=mem;
	bloc_to_free->free=1;
	if(get_header()->first >= bloc_to_free) {
		bloc_to_free->next=get_header()->first;
		get_header()->first = bloc_to_free;
	}
	else{
		struct fb* prev = get_header()->first;
		while(prev->next < bloc_to_free) {
			prev = prev->next;
		}
		bloc_to_free->next=prev->next;
		prev->next=bloc_to_free->next;

		if((long int)bloc_to_free == (long int)(prev) + prev->size + sizeof(struct fb)) {
			struct fb* bloc_to_delete = bloc_to_free;
			printf("Deleting previous block %ld\n", (long int) bloc_to_delete - (long int) get_system_memory_addr());
			prev->next = bloc_to_delete->next;
			prev->size = prev->size + sizeof(struct fb) + bloc_to_delete->size;
			bloc_to_free = prev;
		}
	}
	//On teste si on doit merge deux zones libres l'une à coté de l'autre
	if((long int)bloc_to_free->next == (long int)(bloc_to_free) + bloc_to_free->size + sizeof(struct fb)) {
		struct fb* bloc_to_delete = bloc_to_free->next;
		printf("Deleting next block %ld\n", (long int) bloc_to_delete - (long int) get_system_memory_addr());
		bloc_to_free->next = bloc_to_delete->next;
		bloc_to_free->size = bloc_to_free->size + sizeof(struct fb) + bloc_to_delete->size;
	}
}


struct fb* mem_fit_first(struct fb *list, size_t size) {
	struct fb* bloc = list;
	while(bloc != 0) {
		if(bloc->size == size || bloc->size >= size + sizeof(struct fb)) {
			return bloc;
		}
		bloc = bloc->next;
	}
	return NULL;
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */
	struct fb* bloc= zone-sizeof(struct fb);
	return bloc->size;
	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb* mem_fit_best(struct fb *list, size_t size) {
	return NULL;
}

struct fb* mem_fit_worst(struct fb *list, size_t size) {
	return NULL;
}

