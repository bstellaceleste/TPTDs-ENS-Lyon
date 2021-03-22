/*
 * hm.c
 *
 *  Created on: 18 mars 2020
 *      Author: napster
 */

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include "hm.h"

struct bloc *cache=NULL;
struct bloc_mmap *cache_mmap=NULL;

//Fonction max... Parce que comme d'hab elle est pas dans les fonctions de base
int max(int a, int b)
{
	if(a > b)
		return a;
	return b;
}

//Split d'un bloc. S'il a suffisamment de place pour créer un autre bloc, le fait, sinon rensigne la place eccédentaire comme utilisable par le bloc.
//Retourne un pointeur vers le bloc qu'on a divisé
void *split(struct bloc *to_split, int size){
	to_split->state = 1;
	to_split->donnee = (void *)(((char *)to_split) + sizeof(struct bloc));
	int available = to_split->size_to_malloc - size;

	if(available > sizeof(struct bloc))
	{
		to_split->size_to_malloc = size;
		struct bloc *new_bloc = (struct bloc *)(((char *)to_split) + sizeof(struct bloc) + size);
		new_bloc->size_to_malloc = available - sizeof(struct bloc);
		new_bloc->state = 0;
		new_bloc->next = to_split->next;
		if(to_split->next != NULL)
			to_split->next->prev = new_bloc;
		new_bloc->available = new_bloc->size_to_malloc;
		to_split->next = new_bloc;
		to_split->available = 0;
		new_bloc->prev = to_split;
	}
	else
	{
		to_split->available = available;
	}

	return to_split;
}

//Essaye de fusionner deux blocs consécutifs
void try_to_merge_with_next(struct bloc *to_merge){
	if(to_merge->next == NULL || to_merge->next->state != 0)
		return;
	if((char*)to_merge->next > ((char*)to_merge) + to_merge->size_to_malloc + sizeof(struct bloc))//On vérifie si on est dans des blocs de mémoire contigus
		return;
	struct bloc *victime = to_merge->next;
	to_merge->next = victime->next;
	if(victime->next != NULL)
		victime->next->prev = to_merge;
	to_merge->size_to_malloc += victime->size_to_malloc + sizeof(struct bloc);
	to_merge->available += victime->size_to_malloc + sizeof(struct bloc);
}

/*
 * Pour vous donner une idée, vous avez ceci
 */
 //Initialisation du tas
void init_heap(){
	cache=sbrk(0);
	sbrk(INITIAL_HEAP_SIZE);
	cache->next=NULL;
	cache->prev=NULL;
	cache->size_to_malloc=INITIAL_HEAP_SIZE-sizeof(struct bloc);
	cache->available = cache->size_to_malloc;
	cache->state=0;
}

struct bloc_mmap *create_bloc_mmap(int size, bool pmalloc)//if pmalloc is true then we should insert a guard page after the bloc created
{
	struct bloc_mmap * new_mmap_bloc = (struct bloc_mmap *) mmap(NULL, size + (char *)sizeof(struct bloc_mmap), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0), *tmp;
	
	if(cache_mmap == NULL)
		cache_mmap = new_mmap_bloc;
	else{
		tmp = cache_mmap->next;
		while(tmp->next != NULL)//on sort de la boucle lorsque tmp est le dernier bloc non null
			tmp = tmp->next;
		new_mmap_bloc->next = NULL;
		new_mmap_bloc->size_usable = size;
		new_mmap_bloc->state = 1;
		new_mmap_bloc->prev = tmp;
		tmp->next = new_mmap_bloc;
	}

	return (void *)((char *)new_mmap_bloc + (char *)sizeof(struct bloc_mmap));
}

//Attribution d'un nouveau bloc pour y stocker une donnée de taille size, et renvoie un pointeur vers le début du bloc (méta données). Au besoin, agrandit le tas et crée un nouveau bloc
struct bloc *create_bloc(int size){

	if(cache==NULL)
		init_heap();

	struct bloc *libre = cache;
	struct bloc *dernier = NULL;

	while(libre != NULL && (libre->size_to_malloc < size || libre->state != 0))
	{
		if(libre->next == NULL)
			dernier = libre;
		libre = libre->next;
	}

	if(libre == NULL)
	{
		int t = max(INITIAL_HEAP_SIZE - sizeof(struct bloc), size);
		struct bloc *nouveau_bloc = sbrk(0);
		sbrk(t + sizeof(struct bloc));
		dernier->next = nouveau_bloc;
		nouveau_bloc->prev = dernier;
		nouveau_bloc->donnee = (void *)(((char *)nouveau_bloc) + sizeof(struct bloc));
		nouveau_bloc->state = 1;
		nouveau_bloc->available = t - size;
		nouveau_bloc->size_to_malloc = t;
		return nouveau_bloc;
	}
	else
	{
		return split(libre, size);
	}

	return NULL;
}

//Fonction malloc d'interface, pour éviter de renvoyer l'adresse vers les méta données
void *malloc(int size){
	//printf("Perso\n");
	return (size < MMAP_THRESHOLD) ? create_bloc(size)->donnee : create_bloc_mmap(size, 0);
}

/*
 * Protected malloc for guards pages
 */
void *pmalloc(int size, bool protect)
{
	struct bloc *new_bloc = (struct bloc *) mmap(NULL, size, prot ? PROT_NONE : (PROT_READ | PROT_WRITE, MAP_ANONYMOUS), -1, 0));
	struct bloc *guard_page = (struct bloc *) mmap(NULL, size, PROT_NONE : (PROT_READ | PROT_WRITE, MAP_ANONYMOUS), -1, 0));
}

//Libère le bloc stockant la donnée pointée, et nettoie la mémoire en fusionnant les blocs vides consécutifs et en désallouant l'extrêmité du tas si elle est inutilisée
void free(void *ptr){
	//printf("Perso\n");
	struct bloc *btf = cache;
	while(btf != NULL && btf->donnee != ptr)
		btf = btf->next;
	if(btf == NULL)
		return;
	btf->available = btf->size_to_malloc;
	btf->state = 0;
	try_to_merge_with_next(btf);
	struct bloc *prev = btf->prev;
	if(btf->next == NULL)
	{
		brk(btf);
		if(prev != NULL)
			prev->next = NULL;
		else
			cache = NULL;
	}
	if(prev != NULL)
		try_to_merge_with_next(prev);
}

//Ajuste la taille du bloc. Si suffisamment d'espace a été libéré dans le bloc, crée un nouveau bloc vide dans cet espace.
//Si le bloc actuel s'avère insuffisant, déplace les données dans un autre bloc, qu'il peut créer au besoin.
//Renvoie un pointeur vers les données après éventuel déplacement
void *realloc(void *ptr, int size){
	//printf("Perso\n");
	struct bloc *btr = cache;
	while(btr != NULL && btr->donnee != ptr)
		btr = btr->next;
	if(btr == NULL)
		return NULL;
	if(size < btr->size_to_malloc - btr->available)
	{
		btr->available = btr->size_to_malloc - size;
		if(btr->available > sizeof(struct bloc))
			return split(btr, btr->size_to_malloc - btr->available);
		else
			return ptr;
	}
	else
	{
		if(size <= btr->size_to_malloc)
		{
			btr->available = btr->size_to_malloc - size;
			return ptr;
		}
		else
		{
			struct bloc *new_bloc = create_bloc(size);
			char *new_ptr_char = new_bloc->donnee;
			char *ptr_char = ptr;
			for(int i = 0; i<btr->size_to_malloc; i++)
				(*(new_ptr_char + i)) = (*(ptr_char + i));
			free(ptr);
			return new_bloc->donnee;
		}
	}
	return NULL;
}

