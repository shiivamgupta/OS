#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

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
    //initialisation of the memory and semaphores
    char value;
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
	union semun u1;
	//initialising values of semaphore instances
	u1.val = 1;
	semctl(sem_id,0,SETVAL,u1);		//semaphore instance 1 is binary
	u1.val = 50;
	semctl(sem_id,1,SETVAL,u1);		//counting semaphore
	u1.val = 0;
	semctl(sem_id,2,SETVAL,u1);		//counting semaphore
	//initialising values of shared memory
	shma->rd_index = 0;
	shma->wr_index = 0;
	shma->max_buf_sz = 50;
	shma->used_slot = 0;	
    int k=2,status;
    while(k--){
		ret=fork();
		if(ret<0){
			perror("child cannot be created\n");
			exit(1);
		}
		if(ret>0){
			continue;
		}
		if(ret==0){
			if(k==0){
				execl("/home/desd/Desktop/assignmentsos/ass4/que3/prod","prod",NULL);
				exit(0);
			}
			if(k==1){
				execl("/home/desd/Desktop/assignmentsos/ass4/que3/cons","cons",NULL);
				exit(0);
			}
		}
	}
    
	if(ret>0){
		while(1){
			ret=waitpid(-1,&status,0);
			if(ret>0){
				if(WIFEXITED(status)){
					if(WEXITSTATUS(status)==0){
						printf("success exe normal ter\n");	
					}
					else{
						printf("unsuccess exe normal ter\n");
					}
				}
				else{
					printf("forcible termination\n");
				}
			}
			if(ret<0){
				semctl(sem_id,0,IPC_RMID);
	            shmctl(shm_id,0,IPC_RMID);
	            exit(0);
			}
		}
	}
    
}
