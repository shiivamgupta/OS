#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <error.h>

static int global=10;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* modify(void *arg){
	int error;
	printf("the thread id is : %d\n",arg);
	error=pthread_mutex_lock(&lock);
	if(error==-1)
		exit(1);
	global+=10;
	error=pthread_mutex_unlock(&lock);
       	if(error==-1)
		exit(1);
	pthread_exit(0);	
}

void* display(void *arg){
	int error;
	printf("the thread id is: %d\n",arg);
	error=pthread_mutex_lock(&lock);
	if(error==-1){
		perror("unable to display");
		exit(1);
	}
	printf("displaying a global value: %d\n",global);
	error=pthread_mutex_unlock(&lock);
	if(error==-1)
		exit(1);
	pthread_exit(0);
}

pthread_t pthid1,pthid2,pthid3,pthid4;

int main(){

	int ret;

	//creating 5 threads and each calling either modify or display
	ret=pthread_create(&pthid1,NULL,modify,(void*)1);
	if(ret>0){
		printf("error while creating threads\n");
		exit(2);
	}
	ret=pthread_create(&pthid2,NULL,display,(void*)2);
	if(ret>0){
		perror("error while creating threads\n");
		exit(2);
	}
	ret=pthread_create(&pthid3,NULL,modify,(void*)3);
	if(ret>0){
		perror("error while creathing threads\n");
		exit(2);
	}
	ret=pthread_create(&pthid4,NULL,display,(void*)4);
	if(ret>0){
		perror("error while creathing threads\n");
		exit(2);
	}
	
	//cleaning of the threads and their ids
	pthread_join(pthid1,NULL);
	printf("thread 1 cleaned\n");
	pthread_join(pthid2,NULL);
	printf("thread 2 cleaned\n");
	pthread_join(pthid3,NULL);
	printf("thread 3 cleaned\n");
	pthread_join(pthid4,NULL);
	printf("thread 4 cleaned\n");

	exit(0);
}
