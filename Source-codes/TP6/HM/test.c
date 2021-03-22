#include<stdio.h>
//#include <stdlib.h>

int main(int argc,char **argv){
	printf("Debut\n");
	int *p=(int *)malloc(10);
	if(p==NULL){
		printf("[hm-malloc] Problem de malloc.\n");
	}else{
		printf("[hm-malloc] %d.\n",*p);
	}
	printf("**************\n");
	p=(int *)malloc(30);
	(*p) = 42;
	p = realloc(p, 100);
	if(p==NULL){
		printf("[hm-malloc] Problem de malloc.\n");
	}else{
		printf("[hm-malloc] %d.\n",*p);
	}
	printf("**************\n");
	p=(int *)malloc(50);
	if(p==NULL){
		printf("[hm-malloc] Problem de malloc.\n");
	}else{
		printf("[hm-malloc] %d.\n",*p);
	}
}

