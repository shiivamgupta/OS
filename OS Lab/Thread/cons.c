#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define KEY 1234

pthread_t cons_id;
struct shm_area *shma;

struct shm_area{
	sem_t sembin;
	sem_t semcount;
	pthread_mutex_t mutexcount;
	int rd_index;
	int wr_index;
	int used_slots,max_slots;
	char buffer[10][40];
};

void * cons_thread(void* arg){
	int error;
	char output_str[40];
	printf("Inside consumer\n");
	//while(1){
		while((error=sem_wait(&shma->semcount))==-1 && (errno=EINTR));
		if(error){
			perror("semaphore operation error\n");
			exit(errno);
		}
		while((error=sem_wait(&shma->sembin))==-1 && (errno=EINTR));
		if(error){
			perror("semaphore operation error\n");
			exit(errno);
		}
		if(error=pthread_mutex_lock(&shma->mutexcount)){
			perror("mutex operation error\n");
			exit(error);
		}
		if(shma->used_slots!=0){
			strncpy(output_str,shma->buffer[shma->rd_index],40);
			printf("printing the buffered string\n%s",output_str);
			shma->rd_index= (shma->rd_index + 1) % shma->max_slots;
			shma->used_slots--;
		}
		if(error=pthread_mutex_unlock(&shma->mutexcount)){
			perror("mutex operation error\n");
			exit(error);
		}
		if(error=sem_post(&shma->sembin)){
			perror("semaphore operation error\n");
			exit(error);
		}
	//}
	pthread_exit(0);
}

void main(){
	
	int ret,shm_id;
	
	shm_id=shmget(KEY,4096,IPC_CREAT|0600);
	shma=shmat(shm_id,0,0);

	ret = pthread_create(&cons_id,NULL,cons_thread,NULL);
	pthread_join(cons_id,NULL);
}
