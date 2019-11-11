#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t  mutex_lock = PTHREAD_MUTEX_INITIALIZER;
static sem_t sem_lock;
static sem_t sem_count;
static char buffer[100];

void seminit(void){
	int error;
	error=sem_init(&sem_lock,0,1);		//initialising the semaphore for 
	if(error!=0){
		perror("unable to create or initialise the semaphores\n");
		exit(1);
	}
	else{
		error=sem_init(&sem_count,0,0);
		if(error!=0){
			perror("unable to create or initialise the semaphores\n");
			exit(1);
		}
	}
}

void* modify(void *arg){
	//printf("inside the thread %d\n",arg);
	int error,ret;
	char str[100];
	printf("enter a string:\n");
	ret=fgets(str,sizeof(str),stdin);
	//printf("%d Blocked...\n",arg);

	while((error=sem_wait(&sem_lock))&&(errno==EINTR));
	if(error==-1){
		perror("decrement of semaphore has been interrupted\n");
		exit(errno);
	}	
	error=pthread_mutex_lock(&mutex_lock);
	if(error==-1){
		perror("error while mutex operation\n");
		exit(2);
	}

	strncpy(buffer,str,100);		
	//modified the data inside the buffer
	printf("string successfully copied\n");
		
	error=pthread_mutex_unlock(&mutex_lock);
	if(error==-1){
		perror("error while unlocking the mutex\n");
		exit(2);
	}
	if(sem_post(&sem_lock)==-1){
		perror("error while unblocking semaphores\n");
		exit(errno);
	}
	if(sem_post(&sem_count)==-1){
		perror("error while unblocking semaphores\n");
		exit(errno);
	}

	//printf("%d unblocked\n",arg);
	pthread_exit(0);
}

void* display(void* arg){
	int error;
	//printf("inside the thread %d\n",arg);
	//printf("%d blocked...\n",arg);
	
	while((error=sem_wait(&sem_count))&&(errno==EINTR));
	if(error==-1){
		perror("decrement of semaphore has errorred\n");
		exit(errno);
	}
	while((error=sem_wait(&sem_lock)) && (errno == EINTR));
	if(error==-1){
		perror("error while decrementing the semaphores value\n");
		exit(errno);
	}
	errno = pthread_mutex_lock(&mutex_lock);
	if(error==-1){
		perror("error while mutex operation\n");
		exit(2);
	}
	printf("printing the buffered string\n%s\n",buffer);	
	//display the data inside the buffer
	
	error = pthread_mutex_unlock(&mutex_lock);
	if(error==-1){
		perror("error while mutex operation\n");
		exit(2);
	}
	if(sem_post(&sem_lock)==-1){
		exit(errno);
	}

	//printf("%d unblocked...\n",arg);
	pthread_exit(0);
}

pthread_t thid1,thid2;

int main(){
	seminit();		//initialisation of the semaphores
	
	int ret;
	ret = pthread_create(&thid1,NULL,modify,(void *)1);
	if(ret>0){
		perror("error while initialsation of the semaphores\n");
		exit(3);
	}
	ret = pthread_create(&thid2,NULL,display,(void*)2);
	if(ret>0){
		perror("error while initialsation of the semaphores\n");
		exit(3);
	}
	

	//cleaning the threads
	pthread_join(thid1,NULL);
	pthread_join(thid2,NULL);
	exit(0);
}
