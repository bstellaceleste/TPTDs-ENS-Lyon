/*
 * hm.c
 *
 *  Created on: 18 mars 2020
 *      Author: napster
 */

#include <unistd.h>
#include <stdio.h>
#include "hm.h"

struct bloc *cache=NULL;

void *split(struct bloc *to_split, int size){
	struct bloc *new_bloc=NULL;
	/*
	 * ETAPE 1
	 */
	return new_bloc;
}

void try_to_merge_with_next(struct bloc *to_merge){
	/*
	 * A compléter
	 */
}

/*
 * Pour vous donner une idée, vous avez ceci
 */
void init_heap(){
	cache=sbrk(0);
	sbrk(INITIAL_HEAP_SIZE);
	cache->next=NULL;
	cache->size_to_malloc=INITIAL_HEAP_SIZE-sizeof(struct bloc);
	cache->state=0;
}

void *__malloc(int size){
	if(cache==NULL)
		init_heap();
	/*
	 * ETAPE 1
	 * Parcourir cache
	 */
	return NULL;
}

void __free(void *ptr){
	/*
	 * Parcourir et rechercher le bloc correspondant à ptr
	 * Coller ce bloc avec les suivants et précédents
	 * ETAPE 2
	 */
}

void *__realloc(void *ptr, int size){
	/*
	 * ETAPE 3
	 */
	return NULL;
}
