#include<stdio.h>
#include <stdlib.h>
#include "hm.h"

int main(int argc,char **argv){
	int *p=(int *)__malloc(10);
	if(p==NULL){
		printf("[hm-malloc] Problem de malloc.\n");
	}else{
		printf("[hm-malloc] %d.\n",*p);
	}
	printf("************** %d\n",sizeof(struct bloc));
	p=(int *)__malloc(30);
	if(p==NULL){
		printf("[hm-malloc] Problem de malloc.\n");
	}else{
		printf("[hm-malloc] %d.\n",*p);
	}
	printf("**************\n");
	p=(int *)__malloc(50);
	if(p==NULL){
		printf("[hm-malloc] Problem de malloc.\n");
	}else{
		printf("[hm-malloc] %d.\n",*p);
	}
}

