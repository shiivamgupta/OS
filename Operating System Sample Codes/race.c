#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/shm.h>

#define KEY1 1555

struct shmarea {

  unsigned long count;
  unsigned long count1;
  unsigned long count2;
  unsigned long count3;
  unsigned long count4;

}; 

struct shmarea *shma;

int main()
{

   int ret,status,id;

   unsigned long int i=0;

   id =  shmget(KEY1,4096,IPC_CREAT|0600);
   //id =  shmget(KEY1,getpagesize(),IPC_CREAT|0600);
   //id =  shmget(KEY1,2*2*getpagesize(),IPC_CREAT|0600);

 
   //param2 - 0 means, system allocates virtual addresses for
   //shared virtual pages/VADs !!! you may use non-zero values, but
   //not preferred !!
   //param3 - flags are initially set to 0 !!!
   //we may use flags to set certain specific access permissions !!!
  shma = shmat(id,0,0);  


   shma->count = 3333; 

  while(i++<1){ //change this with a reasonable condition !!!

   ret = fork();
   //after fork(), code/data/heap/stack segments are duplicated
   //using duplicate VADs, which are private VADs
   //however, shared memory segment VADS are duplicated, but
   //set as shared VADs - what does this mean ??
   //when shared segment VADs are used and point to the same
   //shared memory object, their respective pageframes
   //are shared and their ptes are set to point to the 
   //same set of shared page frames !!! 

   if(ret<0){ 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
          
           //an example for normal termination, but not successful 
           exit(1); 
   }

   if(ret>0){ 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu ...and pid is %lu\n", 
		   getppid(),getpid());	   
	   
           while(i++ < 600000) {
              //start of a critical section code block  
    //                        shma->count++;     //ok
                        //    shma->count1++;
                        //    shma->count2++;
              //end of a critical section code block 
           }
           //read the comments below - if you understand the comments,
           //what happens, if we add waitpid() in this block of code,
           //not in the while loop below 
           //if we use waitpid() here, parent process will be blocked
           //until the recently created child process is terminated and
           //cleaned-up - which means, several children processes cannot 
           //co-exit actively - which means, multiprogramming/multitasking
           //is lost - in addition, try to visualize the same problem
           //in a multiprocessing (MP) environment - the efficiency 
           //will be simply unacceptable !!!

           //
           //using waitpid() is not efficient !!!
           //ret=waitpid() 

           continue;
   }

   if(ret==0) { 
	   printf("I am in child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n", 
		   getppid(),getpid());	   
pause();	   

      //     while(i++<600000) shma->count--;
	   
           
           exit(0); 
   }

 }//while

 //this block of code will be executed only by the parent and 
 //parent will reach here only if it has broken the first while loop 
 //and completed its basic work !!!

 //for most cases of process coding, you must use waitpid() as 
 //shown below - in addition, if you observe, this loop is
 //outside the parent's main execution block - meaning, any such
 //clean up activity must be done by the parent after its 
 //actual work - in addition, if you do not code using this 
 //approach, concurrency and multitasking may be lost !!!

 //the code below is a template for the conditions and loop
 //however, modify it as per your understanding and needs !!!
 //the code below is passive - meaning, just prints information 
 //when you code,you may have to take more actions, actively !!! 

 if(ret>0)
 {
   while(1){ //this while(1) is ok - it has been used with a clear purpose
             //it will break when a certain condition is true - see below !!! 

    //as per man page, first parameter can be >0 
    //however, it cannot be an arbitrary no. - it must be +ve 
    //and a valid pid of a child process of this parent process !!!

    //as per man page, first parameter can also be <0 (-ve)
    //absolute value of the -ve value must be a valid 
    //process group id of a set of  children processes of this 
    //parent process !!!

    //as per man page, first parameter can also be 0 - 
    //in which case, this parent process will wait/cleanup
    //any child process belonging to the group id as that of 
    //parent process !! 

    ret = waitpid(-1,&status,0);
    if(ret>0){

    if(WIFEXITED(status))     //normal termination of the process
    {
       if(WEXITSTATUS(status) == 0){ //normal and successfull
       }
       else{//normal, but not successfull
           //you may have to set a flag to be used later 
       }
    }
    else{ //abnormal (did not succeed)
        //you may have to set a flag here to be used later
    }
   }//ret>0
    //we may use break instead of exit(0) 
    if(ret<0) { 
           //pause();

           shmctl(id,IPC_RMID,0); //destroy the shared memory object !!!
// printf("final value of shared counter is %d\n", shma->count); 
           exit(0); 

    } //no child is in any state for this process
                           //all the children have terminated and 
                           //cleaned-up by the parent process
   } //second while 
  }//if after while loop 

return 0;
   
}


