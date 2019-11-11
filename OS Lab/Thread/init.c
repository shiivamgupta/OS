#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>

#define KEY 1234

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

void bufferinit(void){
	int error;
	if(sem_init(&shma->sembin,1,1))
		exit(errno);
	if(sem_init(&shma->semcount,1,0)){
		sem_destroy(&shma->semcount);
        exit(errno);
	}
}

void main(){
	int shm_id;
	shm_id=shmget(KEY,4096,IPC_CREAT|0600);
	shma=shmat(shm_id,0,0);
	shma->rd_index=0;
	shma->wr_index=0;
	shma->used_slots=0;
	shma->max_slots=10;

	bufferinit();
}
