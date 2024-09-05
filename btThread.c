#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "common_threads.h"

void *tree(void *arg);
int gettid();

int 
main(int argc, char *argv[]) 
{
	int ht;

    if (argc != 2) {
		fprintf(stderr, "usage: htree height \n");
		exit(1);
    }

	ht = atoi(argv[1]);

    pthread_t p1;
	Pthread_create(&p1, NULL, tree, &ht);
	Pthread_join(p1, NULL);
    return 0;
}

// It is easier to write a recursive function than an iterative one.
// Remember arg is an address in the stack of the caller.
// I wouldnt modify the value at that location.
void* 
tree(void* arg) 
{
	int height = *(int *)arg;
	if (height == 0){
		printf("Leaf Thread (id: %d) at height %d\n", gettid(), height);
		return NULL;
	}
	printf("Int. Thread (id: %d) at height %d\n", gettid(), height);
	height--;
	pthread_t p1;
	int h1 = height;
	pthread_t p2;
	int h2 = height;
	Pthread_create(&p1, NULL, tree, &h1);
	Pthread_create(&p2, NULL, tree, &h2);
	Pthread_join(p1, NULL);
	Pthread_join(p2, NULL);
	return NULL;
}

int
gettid()
{
	return (syscall(SYS_gettid));
}
