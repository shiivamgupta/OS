#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define THREADS 5

void* thread(void *i){
	
	printf("inside thread %d\n",i);
	printf("caller thread id: %ld\n",pthread_self());
	pthread_exit(0);
}

int main()
{
	int ret;
	pthread_t thid[THREADS];
	
	for(int i=0;i<THREADS;i++){
		//printf("the thread id for %d thread is %ld\n",i,thid[i]);	//prints the same things as the pthread_self()
		ret=pthread_create(thid+i,NULL,thread,(void*)i);
		if(ret>0){
			printf("error while creating threads\n");
			exit(1);
		}
	}
	printf("ending the parent\n");
	
	for(int i=0;i<THREADS;i++){
		printf("cleaning of the processes %d\n",i);
		pthread_join(*(thid+i),NULL);
	}
	pthread_exit(NULL);
}
