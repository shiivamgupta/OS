#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>


#define KEY1 1234


//in the below example, we are implementing an user-space pipe
//mechanism - actually, it is a form of a simple message queue-
//it supports a byte stream of data -  for those who are 
//familiar with Unix/other operating
//systems, system space pipe ipc is very popular !!!
//in this case, the user-space pipe object and buffer will 
//, in user-space - any locking or synchronization needs to 
//be done, in user-space /user-space code   
//in user-space pipe, as well as system space pipe, i
//the ipc buffer used 
//to store/forward data is a circular buffer - you will understand
//the implementation, if you look into the code below !!! 

//the user-space IPC object and its buffer(s) are set-up, 
//in a shared-memory object placed, in a shared-memory 
//region - this shared object/buffers will be used, 
//as an IPC mechanim, for our application/processes 

struct shm_area {
  unsigned short rd_index;  //read index for the circular buffer
  unsigned short wr_index;  //write index for  ""   """

  unsigned short buf_size_max; //max buffer size 
  char buf_area[50]; //this must be treated as circular buffer
  unsigned int used_slot_count;//no elements used in the circular buffer 

  //char my_straread[50][256];   //practice
};

union semun { //used with semctl() system call for initializing 
              //getting the current semaphore count values
  int val;
  unsigned short *array;
  // other fields are omitted

};

int stat_var,ret;

int main()
{

  char value;
  int ret1,ret2,status, shm_id1,sem_id2;
  struct sembuf sb1,sb2,sb3,sb_array[3];

  //in this code/assignment, 3 semaphores are used in a semaphore object 
  //the usage of the semaphores are as below - it is done, 
  //as per applications requirements :

  // first semaphore(0) will be used for counting free slots
  // in the IPC area  - this is a counting semaphore type -
  // as discussed in ipc.txt,for more details refer to ipc.txt  
  //
  // second semaphore is used for lock/mutual exclusion 
  // in the IPC area 
  // refer to ipc.txt - this is a binary semaphore - as 
  // decided by the developer !!! this semaphore is responsible 
  //for locking, in this example context !!!

  //
  //
  // third semaphore is used as another counting semaphore, 
  // for counting filled slots 
  // in the IPC area - this is again a counting semaphore -
  // refe to ipc.txt 
  //
  //in this example, all the semaphore instances are 
  //initialized, using a single semctl() call and 
  //an array is passed, with the respective initial values
  //in this context, semaphore index 0 is set to 50
  //in this context, semaphore index 1 is set to 1
  //in this context, semaphore index 2 is set to 0
  // 
  unsigned short ary1[3],ary[] = { 50,1,0};  
  unsigned short ary2[4],ary3[] = { 50,1,0,0};  

  union semun u1;

  struct shm_area *shma ;


  printf("the address that faults is %x\n", shma); 

  //shma->buf_size_max = 100; 


  shm_id1 = shmget(KEY1,3*sizeof(struct shm_area),IPC_CREAT|0600);
                                         //the last field may be 0 ,
              				 //when the shared memory object
                                         //is already created 
  if(shm_id1<0 && errno != EEXIST) { 

              perror("error in creating the shared memory"); exit(1);
  }

  //we are creating 3 semaphores in a given semaphore object !!!

  //param2 is set to 3 such that 3 semaphores are created in 
  //a semaphore object !!! see the descriptions above, for 
  //usage of semaphores, in this context 


  sem_id2 = semget(KEY1,4,IPC_CREAT | 0600);//with read/write permissions 

  if(sem_id2<0) {

                perror("error in semget");
                exit(2);
  }

  //last field may or may not be 0

  //add error checking

  //shma will now hold the starting virtual address of the 
  //virtual pages allocated for the shared-memory region
  //

  //internally, this will generate VADs, new page table entries
  //and connect VADs to shared memory object - all these objects
  //work together to ensure that shared memory mechanism works
  //appropriately !!!


  

  shma = shmat(shm_id1,0,0);  //address space is assigned by 
                        //kernel , flags are 0
                        // customize access to shared-area

  

  printf("the attached address is 0x%x\n", shma);
  printf("the amount of shm memory used is %d\n", \
	  sizeof(struct shm_area));
 


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary[0], ary[1], ary[2]); 
 //semctl() can be used with several commands - SETVAL is a command
 //used to set a single semaphore's value inside a semaphore object !!!
 //semctl() can also be used with SETALL - SETALL is a command 
 //used to set values of all semaphores, in a semaphore object !!!!
 //when you use SETALL with semctl(), second parameter is ignored -
 //meaning, you cannot use SETALL to initialize a semaphore !!! 
 // 
 //sem_id2 is the first parameter
 //second parameter is ignored 
 //third parameter is the command to set the values of all semaphores
 //in the semaphore object !!!
 //fourth param is pointer to an array of unsigned short elements-
 //each unsigned short element contains corresponding semaphore's
 //initial value - no of elements in this array must equal to
 //total no of semaphore elements in the semaphore object
 //array field of union is used for SETALL command - array field 
 //is used to point to an array of unsigned short elements - 
 //each element in the array initializes one semaphore in the semaphore
 //object !!! 
  u1.array = ary3; //setting the array ptr in the union
  ret1 = semctl(sem_id2,0,SETALL,u1);//setting the semaphore values

  if(ret1<0) { perror("1....error in semctl setting"); exit(4); }

  u1.array = ary3;

  ret1 = semctl(sem_id2,0,GETALL,u1);//getting the semaphore values


  if(ret1<0) { perror("2...error in semctl getting"); exit(5); }


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary1[0], ary1[1], ary1[2]); 

  //we are initializing fields of the IPC object, in 
  //the shared-data/shared-memory segment - since we 
  //dealing, with user-space shared-memory object/
  //IPC, we need to initialize the fields, as per 
  //application's requirements 
  shma->rd_index = 0; 
  shma->wr_index = 0;
  shma->buf_size_max = 50;  
  shma->used_slot_count = 0; 
  
 
   //producer
   //
   while(1)
   {
    
    //in this simple GPOS scenario, we will be using 
    //user-input for filling data into the IPC - meaning, 
    //producer process will read input from the user
    //and send/fill data, in the IPC object/buffer

    //however, in the case of embedded, producer 
    //process will be typically taking input from 
    //an IO device/driver 


    //wait for user input and read the user-input
    //it is a blocking system call
    //
    //in this context, the read() (file/IO system call API)
    //will read input from STDIN_FILENO, which is a 
    //handle/descriptor to the current terminal device file/
    //interface - effectively, reading from the terminal 
    //and user-input - this is basically a blocking system 
    //call API - which means, if user provides some input, 
    //it will be read and otherwise, read() system call API 
    //will block the current process - shell/shell process 
    //also does invoke read() system call API, for reading
    //user-input, in its code/implementation    
  
    ret = read(STDIN_FILENO,&value,1);  //this is good programming
    printf("the value is %c\n", value);
    //decrement freeslot counting semaphore using semop
    //and allocating a free semaphore
    //sb1.sem_num = 0;  //semaphore instance no.
    //sb1.sem_op = -1;  //decrement operation 
    //sb1.sem_flg = 0;  //not using flags
    //semop(sem_id2,&sb1,1);  
     
    
    //decrement mutual exclusion semaphore using semop
    //entering critical section / atomic section
    sb2.sem_num = 1;
    sb2.sem_op = -1;
    sb2.sem_flg = 0;
    semop(sem_id2,&sb2,1); 
   
    //two semaphore operations done atomically
    //
    //sb_array[0].sem_num = 0;//preparing the decrement operation on 
    //sb_array[0].sem_op = -1;//semaphore 0 in the sem.object - 
                            //meaning, use a free slot in producer
    //sb_array[0].sem_flg = 0;

    //sb_array[1].sem_num = 1;   //decrement critical section semaphore 
    //sb_array[1].sem_op = -1;   //acquire semaphore lock !!!
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,sb_array,2);   

    // read(STDIN_FILENO,&value,1); //this is bad programming
    //add an item to the next free slot in 
    //in the circular buffer of the PIPE IPC mechanism
    //the following lines of code do the job of this process
    if(shma->used_slot_count < shma->buf_size_max)
    {
       shma->buf_area[shma->wr_index] = value;
       shma->wr_index = (shma->wr_index+1)%shma->buf_size_max;
       shma->used_slot_count++;
    }
    //two semaphore operations done atomically
    //
    //sb_array[2].sem_num = 1;  //critical section semaphore
    //sb_array[2].sem_op = +1;  //releasing critical section semaphore  
    //sb_array[2].sem_flg = 0;

    //sb_array[1].sem_num = 2;  //incrementing filled slots semaphore count 
    //sb_array[1].sem_op = +1;  //meaning, this semaphore maintains the count
                              //of filled slots - this is use ful in the
                              //context of consumer 
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,&sb_array[1],2);   

    //increment  mutual exclusion semaphore using semop
    //we are leaving the critical section
    sb2.sem_num = 1;  //we are operating on the second semaphore
    sb2.sem_op = +1;  //increment
    sb2.sem_flg = 0;
    semop(sem_id2,&sb2,1); 

    //increment filled slot counting semaphore using semop
    //
    //sb1.sem_num = 2;
    //sb1.sem_op = +1;
    //sb1.sem_flg = 0;
    //semop(sem_id2,&sb1,1);

    //sched_yield();  //choice-optional 
   }//loop	  


   exit(0); //keep the compile happy 

}




