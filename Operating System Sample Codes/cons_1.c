#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>


//consumer process code, which uses the same 
//shared memory object/data/a common set of 
//semaphores, along with producer 

//as per this set-up, producer process/code must 
//be executed first, before the consumer process/code
//for such a set-up to work correctly, we need to 
//write a simple script and use it for launching/
//testing producer/consumer model(s)  
//
//in any producer-consumer design/model,we must 
//ensure that there is only one instance of 
//initialization, in producer or consumer, not 
//in both - in most of our examples, initialization
//will be done, in the producer  
//
//
//there can be different scenarios - in one, there 
//can be a single producer process and a single 
//consumer process
//
//there can be another scenario, where there can be 
//multiple producers and a single consumer 
//
//
//
//
#define KEY1 1234

struct shm_area {
  unsigned short rd_index;
  unsigned short wr_index;
  unsigned short buf_size_max;
  char buf_area[50]; //this must be treated as circular buffer
  unsigned int used_slot_count;

  //char my_straread[50][256];   //practice
};

union semun { //used with semctl() system call for initializing 
              //getting the current semaphore count values
  int val;
  unsigned short *array;
  // other fields are omitted

};

int stat_var;

int main()
{

  char value;
  int ret1,ret2,status, shm_id1,sem_id2;
  struct sembuf sb1,sb2,sb3,sb_array[3];

  // first semaphore(0) will be used for counting free slots
  // in the IPC area
  //
  // second semaphore is used for mutual exclusion 
  // in the IPC area 
  //
  // third semaphore is used for counting filled slots 
  // in the IPC area 
  //
  unsigned short ary1[3],ary[] = { 50,1,0};  

  union semun u1;

  struct shm_area *shma ;


  printf("the address that faults is %x\n", shma); 

  //shma->buf_size_max = 100; 

  //in the consumer, the same shared-memory object/
  //data is accessed - the system will provide the 
  //same id of the common shared-memory object/data 
  //
  //in this context, shmget() will return id of the 
  //common shared-memory object, that was already 
  //created,in the producer 
  //
  //
  //
  shm_id1 = shmget(KEY1,sizeof(struct shm_area),IPC_CREAT|0600);
                                         //the last field may be 0 ,
              				 //when the shared memory object
                                         //is already created 
  if(shm_id1<0 && errno != EEXIST) { 

              perror("error in creating the shared memory"); exit(1);
  }


  //here, we will be accessing the same,common
  //semaphores, for counting resources and 
  //protecting critical section(s)
  //
  //we get the id of the semphore object and operate 
  //on semaphore instances 
  sem_id2 = semget(KEY1,3,IPC_CREAT | 0600);//with read/write permissions 

  if(sem_id2<0) {

                perror("error in semget");
                exit(2);
  }

  //last field may or may not be 0

  //add error checking

  //shma will now hold the starting virtual address of the 
  //virtual pages allocated for the shared-memory region
  //
  //since the producer and consumer processes are 
  //unrelated, separate shared segments need to be 
  //set-up, explicitly, using system call APIs 
  

  shma = shmat(shm_id1,0,0);  //address space is assigned by 
                        //kernel , flags are 0
                        // customize access to shared-area

  //after this, no other initialization actions must 
  //done, in the consumer 

  printf("the attached address is 0x%x\n", shma);
  printf("the amount of shm memory used is %d\n", \
	  sizeof(struct shm_area));
 


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary[0], ary[1], ary[2]); 
  



  u1.array = ary1;

  ret1 = semctl(sem_id2,0,GETALL,u1);//getting the semaphore values


  if(ret1<0) { perror("error in semctl getting"); exit(5); }


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary1[0], ary1[1], ary1[2]); 
  
    while(1)
    {
        //consumer

    //two semaphore operations done atomically
    //
    //sb_array[0].sem_num = 2;  //third semaphore is a counting semaphore for filled slots
    //sb_array[0].sem_op = -1;
    //sb_array[0].sem_flg = 0;

    //sb_array[1].sem_num = 1;   //second semaphore is a critical section semaphore 
    //sb_array[1].sem_op = -1;
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,sb_array,2);   

    sb_array[1].sem_num = 1;   //second semaphore is a critical section semaphore 
    sb_array[1].sem_op = -1;
    sb_array[1].sem_flg = 0;
 
    semop(sem_id2,&sb_array[1],1);  
    // read(STDIN_FILENO,&value,1); //this is bad programming
    //add an item to the next free slot in 
    //in the circular buffer
    //extract data from a filled slot of the IPC buffers, 
    //if there is data - in this context, there is not 
    //much processing, but in a real-world IO, there
    //will be specific processing 
    if(shma->used_slot_count != 0)
    {
      value = shma->buf_area[shma->rd_index];
      printf("the new value is %c\n",value); 
      shma->rd_index = (shma->rd_index+1)%shma->buf_size_max;
      shma->used_slot_count--;
    }
    //two semaphore operations done atomically
    //
    //sb_array[2].sem_num = 1;  //critical section semaphore
    //sb_array[2].sem_op = +1;
    //sb_array[2].sem_flg = 0;

    //sb_array[1].sem_num = 0;
    //sb_array[1].sem_op = +1;
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,&sb_array[1],2);   


    sb_array[2].sem_num = 1;  //critical section semaphore
    sb_array[2].sem_op = +1;
    sb_array[2].sem_flg = 0;

 
    semop(sem_id2,&sb_array[2],1);  

     //sched_yield(); //optional - as per the application's req. 
   }//consumer while



   exit(0);

}




