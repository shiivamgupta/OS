#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <string.h>

#define KEY1 1222
#define KEY2 1333

struct shm_area{
	unsigned int rd_index;
	unsigned int wr_index;
	unsigned int max_buf_sz;
	char buf_fil[50][32];
	unsigned int used_slot;
};

union semun{
	int val;
	unsigned int *array;
};

int main()
{
	char value[32]; //
	int ret,sem_id,shm_id;
	struct sembuf sb1,sb2,sb3,sb_arr[3];
	//unsigned int arr[3]={1,50,0};
	
	//shared memory
	shm_id = shmget(KEY1,4096,IPC_CREAT|0600);
	if(shm_id < 0 && errno != EEXIST){
		perror("unable to create shared memory\n");
		exit(5);
	}
	struct shm_area *shma;
	shma = shmat(shm_id,0,0);

	//semaphore 
	sem_id = semget(KEY2,3,IPC_CREAT|0600);
    if(sem_id < 0){
        perror("unable to create semaphore obj\n");
		exit(5);
    }
    printf("inside producer\n");
	//producer
	while(1){
		
            ret = read(STDIN_FILENO,value,32);
            value[ret] = '\0';
		    sb_arr[0].sem_num = 0;
        	sb_arr[0].sem_op = -1;
        	sb_arr[0].sem_flg = 0;

        	sb_arr[1].sem_num = 1;
        	sb_arr[1].sem_op = -1;
        	sb_arr[1].sem_flg = 0;
		    semop(sem_id,sb_arr,2);
		
        if(shma->used_slot < shma->max_buf_sz){
			    strcpy(shma->buf_fil[shma->wr_index],value);
                printf("2.the string is %s\n", shma->buf_fil[shma->wr_index]);
			    shma->wr_index = (shma->wr_index +1) % shma->max_buf_sz;
		        shma->used_slot++;	
		}
		
        	sb_arr[0].sem_num = 0;
        	sb_arr[0].sem_op = +1;
        	sb_arr[0].sem_flg = 0;
		
		    sb_arr[1].sem_num = 2;
        	sb_arr[1].sem_op = +1;
        	sb_arr[1].sem_flg = 0;
		    semop(sem_id,sb_arr,2);
		
		//sched_yield();			//goto customer
	}
}
