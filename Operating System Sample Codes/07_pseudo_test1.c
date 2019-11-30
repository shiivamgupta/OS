
//
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<sched.h>
#include<sys/time.h>
#include<sys/resource.h>


//
//
static buffer_t buffer[5][BUFSIZE];
static buffer_t user_input_buffer[1024];
static pthread_mutex_t  input_buffer_lock   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  data_buffer_lock    = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  display_buffer_lock = PTHREAD_MUTEX_INITIALIZER;
static int bufin = 0;
static int bufout = 0;
static int freeslots = 0;
static int filledslots = 0;
static volatile sig_atomic_t initdone = 0;
static int initerror = 0;
static pthread_once_t initonce = PTHREAD_ONCE_INIT;
static sem_t semitems;
static sem_t semslots;
static sem_t seminput;

//
//
static int bufferinit(void) { /* called exactly once by getitem and putitem  */
   int error;
   if (sem_init(&semitems, 0, 0))    //counting and synchronization semaphore
      return errno;
   if (sem_init(&semslots, 0, 5)) {  //counting and synchronization    
      error =errno;
      sem_destroy(&semitems);
   }                                      /* free the other semaphore */
   if (sem_init(&seminput, 0, 0)) {  //synchronization semaphore 
      error = errno;
      sem_destroy(&semslots);      return error;
      sem_destroy(&semitems);      return error;
   }
   return 0;
}

//
//
static void initialization(void) {
   initerror = bufferinit();
   if (!initerror)
      initdone = 1;
}

//
//
static int bufferinitonce(void) {          /* initialize buffer at most once */
   int error;
   if ((error = pthread_once(&initonce, initialization)) )
      return error;
   return initerror;
}

//consumer
//
int getitem(buffer_t *item) {  /* remove item from buffer and put in *itemp */
   int error;
 //  if (!initdone)
 //     bufferinitonce();
   while (((error = sem_wait(&semitems)) == -1) && (errno == EINTR)) ;
   if (error)
      return errno;
   if ((error = pthread_mutex_lock(&data_buffer_lock)))
      return error; 
   strncpy(item,buffer[bufout],1024);  //copy string
   //*itemp = buffer[bufout];
   bufout = (bufout + 1) % 5;

   filledslots--;
   freeslots++;  

   printf("2..get item \n");
   if ((error = pthread_mutex_unlock(&data_buffer_lock)))
      return error;
   if (sem_post(&semslots) == -1)  
      return errno; 
   return 0; 
}

//producer 
//
int putitem(buffer_t *item) {                    /* insert item in the buffer */
   int error;
 //  if (!initdone)
 //     bufferinitonce();
   while (((error = sem_wait(&semslots)) == -1) && (errno == EINTR)) ;
   if (error)
      return errno;
   if ((error = pthread_mutex_lock(&data_buffer_lock)))
      return error;    
   //buffer[bufin] = item;

   strncpy(buffer[bufin],item,1024);  //copy string
   bufin = (bufin + 1) % 5;
   
   filledslots++;
   freeslots--; 
   printf("1..put item \n");
   if ((error = pthread_mutex_unlock(&data_buffer_lock)))
      return error; 
   if (sem_post(&semitems) == -1)  
      return errno; 
   return 0; 
}


//
//
void *user_input_thread(void *arg)
{
  
   int ret;

   char pdatabuf[1024];

   while(1){

     printf("please input a maximum of 1023 character string - :)");
     fflush(stdout);
     
     ret = fgets(pdatabuf,sizeof(pdatabuf),stdin);    //produce
     if(ret == 0) exit(3); //fatal error
 
     //printf("data input is %s\n", pdatabuf);
 
     pthread_mutex_lock(&input_buffer_lock);

     strncpy(user_input_buffer,pdatabuf,sizeof(user_input_buffer));

     pthread_mutex_unlock(&input_buffer_lock);

     //printf("data input is %s\n", user_input_buffer);
     sem_post(&seminput);
    
     //sched_yield();


   }   

pthread_exit(NULL);

}


void *producer_thread(void *arg)
{
  
   int ret;
   char pdatabuf[1024];


   while(1){

     //fgets(pdatabuf, sizeof(pdatabuf), stdin);                 //produce

     sem_wait(&seminput);     //synchronization of this thread
                              //with user input thread !!!

     pthread_mutex_lock(&input_buffer_lock);

     ret = putitem(user_input_buffer);

     pthread_mutex_unlock(&input_buffer_lock);

     if(ret != 0) exit(3); //fatal error

   }   

pthread_exit(NULL);

} 

//prototype of a thread method is decided by the library 
//may take a ptr as a parameter - may be NULL
//may return a ptr as return value - may be NULL
//typically, threads may execute in a while loop 
//however, they are typically blocked/woken-up as needed
//in certain cases, they may be periodic 
//in certain cases, they may have a time-out associated with
//them
//whatever be the case, they may be forcibly terminated 
//,if needed - there is an API known as pthread_cancel()
//for this - in addition, signals/fatal signals may also
//be used to terminate a thread or a set of threads as
//needed !!!
//
//
void *consumer_thread(void *arg)
{
    
   int ret;
   char cdatabuf[1024];

   while(1){

        ret = getitem(cdatabuf);                             //consume
        if(ret != 0) exit(4); //fatal error
        //printf("the received data is %s\n", cdatabuf);
   }

pthread_exit(NULL);

} 

void *display_status_thread(void *arg)
{
    
   int ret;
   char cdatabuf[1024];

   sigset_t set1;

   sigemptyset(&set1);
   sigaddset(&set1,SIGINT);
   sigaddset(&set1,SIGTERM);

   //refer to man syscall and man gettid !!
   //to change non realtime priority !!!
   //you may find that there are certain thread APIs missing and 
   //in these cases, we may have to use tricks  and certain 
   //system call APIs and complete the job !!!
   //in this context, the main trick is using system space
   //thread id, not process id ???
   setpriority(PRIO_PROCESS, syscall(SYS_gettid) , +19); 


   //in this context, we are selectively unmasking 
   //signal bits - one of the best uses for SIG_UNBLOCK !!!
   //in this context, signal bits that are set in the 
   //user-space mask are unmasked in system space mask field
   //of the thread - other signal bits of the private 
   //signal mask of the thread will not be disturbed -
   //meaning, private mask field of the thread is affected as required !!
   //
   //this thread will be handling certain non-fatal signals,
   //on behalf of this process !!!
   pthread_sigmask(SIG_UNBLOCK,&set1,NULL);

   while(1){

        pthread_mutex_lock(&data_buffer_lock);

        printf("the free slots is %d \n and filled slots is  %d\n",\
                freeslots,filledslots );
        pthread_mutex_unlock(&data_buffer_lock);
        
       //it is better to block this thread for a certain time
       //out and wake it up - this will ensure that the thread
       //does not waste cpu cycles, when it is awarded a time
       //share - in addition, we must not hold a lock and
       //block - some thing very simple, but causes many 
       //problems in a multitasking application !!!
   
      //sleep(5) internally invokes certain system call APIs
      //and blocks current process in the timer subsystem - 
      //in addition, timer subsystem will wake-up a blocked
      //process after the given time-out - in addition, timer 
      //subsystem works with the help of hw timer !!!

        // sleep(5); 
   }

pthread_exit(NULL);

}


//
//
//user-space thread library ids - these are not same as 
//system space thread ids - do not mix up - however, 
//thread library APIs take care of hiding the abstraction !!!
//internally, these IDs may be mapped with system call APIs !!!

//user-space thread IDs are abstract and opaque !!!

pthread_t thid1,thid2,thid3,thid4;
int main()
{

   int ret;
   void *ptr = NULL;
   sigset_t set1,set2;

   struct sched_param param1;

   //these are abstract thread attr objects - 
   //these can be local variables and
   //they must be initialized before using them !!!!
   pthread_attr_t pthread_attr1,pthread_attr4;

   bufferinit(); //to initialize semaphores 

   sigfillset(&set1);

   //here, we are enabling every signal's mask bit !!!
   sigprocmask(SIG_BLOCK,&set1,&set2);
   //every sibling thread inherits masks from the creating thread !!!
  
   //setpriority(PRIO_PROCESS,getpid(), +19); 


   //install all the signal handlers here-before any thread is 
   //created
   //this work is pending ???

   //in this context, we are interested in modifying the user-space stack 
   //of a thread as below - we are allocating the custom stack using 
   //malloc() - you are free to use mmap() for the same - this is a 
   //simpler example - refer to prodcons_threads2.c for mmap() example !!! 

   pdfd0  = open("/dev/pseudo0", O_RDWR); 
   //pdfd0  = open("/dev/pseudo0", O_RDWR|O_NONBLOCK); //device file accessed in non-blocking mode
   if(pdfd0<0) { perror("error in opening first device"); exit(1);}

   pdfd1  = open("/dev/pseudo1", O_RDWR); 
   //pdfd1  = open("/dev/pseudo1", O_RDWR|O_NONBLOCK); //accessed in non-blocking mode 
   if(pdfd0<0) { perror("error in opening first device"); exit(1);}
   if(pdfd1<0) { perror("error in opening second device"); exit(2);}

   ret = pthread_create(&pthread1,NULL,read_method,(void*)pdfd0);
   if(ret>0) { printf("error in thread creation for producer\n"); exit(4); }   

   ret = pthread_create(&pthread2,NULL,write_method,(void *)pdfd0);
   if(ret>0) { printf("error in thread creation for producer\n"); exit(4); }   

   ret = pthread_create(&pthread3,NULL,read_method,(void*)pdfd1);
   if(ret>0) { printf("error in thread creation for producer\n"); exit(4); }   

   ret = pthread_create(&pthread4,NULL,write_method,(void*)pdfd1);
   if(ret>0) { printf("error in thread creation for producer\n"); exit(4); }  

   ptr = malloc(32*1024);
   if(ptr!=NULL){

      printf("setting the stack attributes of a thread\n"); 
      
      //initialization of the thread attribute obj. is a must
      //before using the attribute object in the pthread_create()

      //after initializing the thread attr object, you can set the
      //attributes as per your requirements 

      //still, initializing thread attr object ensures that 
      //all attributes are initially set to default system settings
      //for more details on default settings, refer to manual pages !!!
      ret = pthread_attr_init(&pthread_attr4); 
      if(ret>0) { 
              printf("error in the thread attr initialization\n");  
      }
      //initialization of the stack attributes in the thread attribute obj.
      ret = pthread_attr_setstack(&pthread_attr4,ptr,32*1024);
      if(ret>0) {

              printf("error in the stack attribute settings\n");
      }

   }


   //once attribute object is properly initialized and setup for
   //individual attribute changes, you can pass the attribute object
   //to pthread_create() as below - library API will use the 
   //attributes and may also pass these attributes to 
   //system call APIs, if needed !!!

   //after a new thread is created by the system, all we can say is
   //that it will be added to ready queue of the current processor
   //or another processor , depending upon system configuration !!
   //order of execution of threads is still unpredictable in the 
   //current context of discussion and policies used !!!

   //by default, a new td and other system space resources are 
   //allocated for this thread using underlying system call !!!
   //in addition, user-space stack is allocated explicitly by 
   //the threading library and passed to underlying system 
   //call - user space thread library has its own default value
   //and this default value differs from one implementation to 
   //another implementation !!!
   //there are cases where developer may have to fine tune
   //the thread creation attributes to manage several threads
   //efficiently - for instance, we can fine tune the thread
   //user-space stacks such that more threads can be accomodated
   //within a given process !!! 
   //internally, pthread_create() invokes clone() system call API,
   //using indirect techniques !!!
   ret = pthread_create(&thid4,&pthread_attr4,display_status_thread,NULL);

   if(ret>0) { printf("error in thread creation for producer\n"); exit(4); }   

  //apis below cannot be used to change non realtime priorities !!!

   //another attrib object - must be initialized 
   pthread_attr_init(&pthread_attr1); 

   //setting the policy to realtime, priority based 
   pthread_attr_setschedpolicy(&pthread_attr1,SCHED_FIFO);

   //realtime priority of 1 ( 1-99 is the range) 
   param1.sched_priority = 1;
   pthread_attr_setschedparam(&pthread_attr1, &param1);  
   

   //you must set the following attribute, if you wish to use
   //explicit scheduling parameters - if you do not, system 
   //will ignore the explicit scheduling parameters passed
   //via attrib object and inherit the scheduling parameters
   //of the creating sibling thread !!! 
   pthread_attr_setinheritsched(&pthread_attr1,PTHREAD_EXPLICIT_SCHED); 


   ret = pthread_create(&thid1,&pthread_attr1,user_input_thread,NULL);
   if(ret>0) { printf("error in thread creation for consumer\n"); exit(1); }

   ret = pthread_create(&thid2,NULL,consumer_thread,NULL);
   if(ret>0) { printf("error in thread creation for consumer\n"); exit(2); } 

   ret = pthread_create(&thid3,NULL,producer_thread,NULL);
   if(ret>0) { printf("error in thread creation for producer\n"); exit(3); } 

   //a thread that is created is generally created as a joinable
   //thread - meaning, a sibling thread(mostly main) must join/wait
   //for a newly created thread - this is a natural step - if you miss
   //this your application's main may terminate and eventually, all
   //your threads may prematurely terminate - there is no point 
   //in doing so !!! - however, there are cases where a detached 
   //thread may be created, if needed - in this case, a detached
   //thread cannot be joined/waited upon - it is purely upto the
   //developer to decide and implement the threads as joinable or
   //detached !!! by default, in most cases, it preferred to 
   //maintain joinable threads - for a joinable thread, we can also
   //collect any return information (ptr) returned by the corresponding
   //thread !!!!
   // 
   pthread_join(thid1,NULL);

   pthread_join(thid2,NULL);

   pthread_join(thid3,NULL);
   
   pthread_join(thid4,NULL);

   exit(0);

}






