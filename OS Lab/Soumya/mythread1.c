#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define THREADS 5

static int global;
static pthread_mutex_t input = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t display = PTHREAD_MUTEX_INITIALIZER;

void* glb_input(void *i){
	printf("inside thread %d\n",i);
	printf("caller thread id: %ld\n",pthread_self());
	pthread_exit(0);
}

pthread_t thid1,thid2,thid3,thid4;

int main()
{
	int ret;
	
	//printf("the thread id for %d thread is %ld\n",i,thid[i]);	
	//prints the same things as the pthread_self()
	ret=pthread_create(thid+i,NULL,thread,(void*)i);
	if(ret>0){
		printf("error while creating threads\n");
		exit(1);

	printf("ending the parent\n");
	
	for(int i=0;i<THREADS;i++){
		printf("cleaning of the processes %d\n",i);
		pthread_join(*(thid+i),NULL);
	}
	pthread_exit(NULL);
}
