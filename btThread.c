/*
 * Written by Adrian on 04/01/2024
*/

#include <stdio.h>     
#include <stdlib.h>   
#include <stdint.h>  
#include <inttypes.h>  
#include <errno.h>     // for EINTR
#include <fcntl.h>     
#include <unistd.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include "common.h" //gettime
#include "common_threads.h" //thread assertion


// Print out the usage of the program and exit.
void Usage(char*);
//Hash function
uint32_t jenkins_one_at_a_time_hash(const uint8_t* , uint64_t );
//Thread to hash the file
void * hashTree(void *);
// block size
#define BSIZE 4096

//Global variables for the hash thread
uint32_t nodeNumber; //target number of nodes in binary tree
uint8_t *memmap; //pointer to mapped memory
uint64_t blockBytes; //length (in bytes) needed to be hashed by each thread

int 
main(int argc, char** argv) 
{
  int32_t fd;
  uint32_t nblocks;

  // input checking 
  if (argc != 3)
    Usage(argv[0]);

  // open input file
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  //Input checking
  if (atoi(argv[2]) <= 0){
  	printf("Error: Invalid number of threads\n");
	exit(EXIT_FAILURE);
  }
  //Strcut for fstat
  struct stat st;
  fstat(fd, &st);
  //Num of blocks that each thread will hash
  nblocks = st.st_size / BSIZE / atoi(argv[2]) ; 
  printf(" no. of blocks per thread = %u \n", nblocks);
  uint8_t *arr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  nodeNumber = atoi(argv[2]) ;
  memmap = arr;
  //Convert nblocks into bytes
  blockBytes = (uint64_t)nblocks * BSIZE;
  double start = GetTime();
  
  // calculate hash value of the input file
  pthread_t parent;
  //Pointer to return value of the thread
  void *hash = malloc(sizeof(uint32_t));
  //Number of the root node
  uint32_t i = 0;
  Pthread_create(&parent, NULL, hashTree, &i);
  Pthread_join(parent, &hash);
  
  double end = GetTime();
  printf("hash value = %u \n", *(uint32_t *)hash);
  printf("time taken = %f \n", (end - start));
  close(fd);
  return EXIT_SUCCESS;
}

void *hashTree(void *arg){
	char buffer[50]; //holds the cancatinated string of hash type here+left+right;
	pthread_t pleft; //left child process
	pthread_t pright; //right child process
	void *left = NULL; //for retval of left child
	void *right = NULL; //for retval of right child
	uint32_t leftHash = 0; //retval of left child as a non-pointer type
	uint32_t rightHash = 0; //retval of right child as a non-pointer type
	uint32_t node = *(uint32_t *)arg; //Identify which node we are in
	uint8_t *offset = memmap + node * blockBytes; //Calculate offset
	uint32_t *parent = malloc(sizeof(uint32_t)); //Allocate memory for the retval of the parent
	uint32_t parentHash = jenkins_one_at_a_time_hash(offset, blockBytes); //calc the hash
	*parent = parentHash; //Save the value in allocated memory
	//If needed, create a left child
	if (node*2 + 1 < nodeNumber){
		//Allocate memory for return value
		left = malloc(sizeof(uint32_t));
		uint32_t leftNodeNumber = node*2+1;
		//Make a left child
		Pthread_create(&pleft, NULL, hashTree, &leftNodeNumber);
	}
	//If needed, create the right child
	if (node*2 + 2 < nodeNumber){
		//Allocate memory for the return value
		right = malloc(sizeof(uint32_t));
		uint32_t rightNodeNumber = node*2+2;
		//Make a right child
		Pthread_create(&pright, NULL, hashTree, &rightNodeNumber);
	}
	//If two children were created
	if(node*2 +2 < nodeNumber){
		//Join the threads, save the retvals as ints, free the pointers
		Pthread_join(pright, &right);
		rightHash = *(uint32_t *)right;
		free(right);
		Pthread_join(pleft,&left);
		leftHash = *(uint32_t *)left;
		free(left);
		//Concatenate the resulting 3 hashes and rehash
		sprintf(buffer, "%u%u%u%c", parentHash, leftHash, rightHash, '\0');
		uint32_t hashof3 = jenkins_one_at_a_time_hash((uint8_t *)buffer, strlen(buffer));
		//Allocate the space for the retval and exit
		uint32_t *total = malloc(sizeof(uint32_t));
		*total = hashof3;
		pthread_exit(total);
	//If only left child was created
	} else if (node*2+1< nodeNumber){
		//Join the thread, save the retval as an int, free ther pointer
		Pthread_join(pleft,&left);
		leftHash = *(uint32_t *)left;
		//Concatenate the resulting 2 hashes and rehash
		sprintf(buffer, "%u%u%c", parentHash, leftHash, '\0');
		uint32_t hashof2 = jenkins_one_at_a_time_hash((uint8_t *)buffer, strlen(buffer));
		//Allocate the space for the retval and exit
		uint32_t *total = malloc(sizeof(uint32_t));
		*total = hashof2;
		pthread_exit(total);
	//If a leaf, exit with its hash
	} else{
		pthread_exit(parent);
	}
}

uint32_t 
jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length) 
{
  uint64_t i = 0;
  uint32_t hash = 0;

  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}


void 
Usage(char* s) 
{
  fprintf(stderr, "Usage: %s filename num_threads \n", s);
  exit(EXIT_FAILURE);
}
