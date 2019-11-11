#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define KEY 1234

pthread_t prod_id;
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

void * prod_thread(void* arg){
	int error,ret;
	char input_str[40];
	//while(1){
	    printf("Enter the string:\n");
		ret=fgets(input_str,sizeof(input_str),stdin);
		if(ret==0){
			perror("fgets operation error\n");
			exit(3);
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
		if(shma->used_slots < shma->max_slots){
			strncpy(shma->buffer[shma->wr_index],input_str,40);
			printf("entered string is %s\n",shma->buffer[shma->wr_index]);
			shma->wr_index= (shma->wr_index + 1) % shma->max_slots;
			shma->used_slots++;
		}
		if(error=sem_post(&shma->semcount)){
			perror("counting semaphore error\n");
			exit(error);
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

	ret = pthread_create(&prod_id,NULL,prod_thread,NULL);
	pthread_join(prod_id,NULL);
}
