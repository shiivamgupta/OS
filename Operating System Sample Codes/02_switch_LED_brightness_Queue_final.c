
//
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<sched.h>
#include<sys/time.h>
#include<sys/resource.h>
#include <semaphore.h>

/* Used as a loop counter to create a very crude delay. */
#define mainDELAY_LOOP_COUNT            ( 0xfffffff )

#define TRUE  				( 1 )
#define FALSE 				( 0 )

#define CLK_NANO_SEC_OVERFLOW           ( 1000000000 )
#define CLK_REQ_PERIOD_SEC              ( 0 )

#define CLK_SWITCH_DEBOUNCE_PERIOD_NANO_SEC 	( 2000000 )     //2ms

#define PWM_PERIOD_NANO_SEC                     ( 10000000 ) 	//10ms. 
							     	//Note: Choose any value until the LED flickering is not invisible 
#define PWM_RESOL_NANO_SEC			( 1000000 )

#define DUTY_CYCLE_MAX				( PWM_PERIOD_NANO_SEC )

#define USERSPACE_QUEUE_SIZE			( 10 )

unsigned long x = 0, y = 0, z = 0;
int pdfd0, pdfd1, pdfd2;
static sem_t sem_filledslots;
static sem_t sem_freeslots;
static sem_t sem_buttonpress;
unsigned int userspace_queue[USERSPACE_QUEUE_SIZE];
unsigned int q_in = 0;
unsigned int q_out = 0;

unsigned int next_ON_time = 0;

//a mutex object must be a global object in the process
//a mutex object is also an abstract object of the thread library 
//initialize and use it using thread APIs 

//in user-space !!
pthread_mutex_t m1;   //instantiate as many as you need !!!
pthread_mutex_t m2;   //instantiate as many as you need !!!

void led_ON(int fd)
{
	lseek(fd, 0, SEEK_SET);
        write(fd, "1", 1);
}

void led_OFF(int fd)
{
	lseek(fd, 0, SEEK_SET);
        write(fd, "0", 1);
}

//char g_led1_rd_buf[2]; //good enough to hold the '1' or '0' and '\0'
void ledToggle(int fd)
{
        //char rd_buf[2], wr_buf[2]; //good enough to hold the '1' or '0' and '\0'
        int ret;
	char *rd_buf;
/*      pdfd0  = open("/sys/bus/platform/devices/custom_leds/leds/led1", O_RDWR); 
        //pdfd0  = open("/dev/pseudo0", O_RDWR|O_NONBLOCK); //device file accessed in non-blocking mode
        if(pdfd0<0) { 
                perror("error in opening first device"); 
                exit(1);
        }*/

	rd_buf = malloc(2);

        ret = lseek(fd, 0, SEEK_SET);
        //ret = lseek(fd, 0, SEEK_CUR);
        //printf("1.lseek ret = %d\n", ret);
        ret = read(fd, rd_buf, 1);
        //ret = read(fd, g_led1_rd_buf, 1);
        if (ret != 1) {
                printf("error occurred\n");
                printf("ret = %d\n", ret);
        }
        else {
        //      printf("ret = %d\n", ret);
        //      printf("rd_buf = %s\n", rd_buf);
        //      printf("rd_buf = %s\n", g_led1_rd_buf);
              ret = lseek(fd, 0, SEEK_SET);
        //      ret = lseek(fd, 0, SEEK_CUR);
        //      printf("2.lseek ret = %d\n", ret);

        //      if(strcmp(g_led1_rd_buf, "1") == 0) {  //current state is ON
        	if(strcmp(rd_buf, "1") == 0) {  //current state is ON
        //              *wr_buf = '0';
        //              *(wr_buf+1) = '\0';
        //              write(fd, wr_buf, 1);
                        write(fd, "0", 1);
                }
                else {
        //              *wr_buf = '1';
        //              *(wr_buf+1) = '\0';
        //              write(fd, wr_buf, 1);
                        write(fd, "1", 1);
                }
        }
	
	free(rd_buf);
//      close(pdfd0);

}

char sw_rd_buf[2];
int buttonDebounce(int fd)
{
	static unsigned short buttonState = 0;
	unsigned char pinState;
	//char *rd_buf;
	int ret;

	//rd_buf = malloc(2);

	//pinState = Bread_Board_SW_Read( SWITCH2 );
        ret = lseek(fd, 0, SEEK_SET);
        //ret = lseek(fd, 0, SEEK_CUR);
        //printf("1.lseek ret = %d\n", ret);
        ret = read(fd, sw_rd_buf, 1);
        //ret = read(fd, g_led1_rd_buf, 1);
        if (ret != 1) {
                printf("error occurred\n");
                printf("ret = %d\n", ret);
        }

	pinState = (unsigned char)strtol(sw_rd_buf, (char **)NULL, 10);

	//free(rd_buf);

	//printf("pinState = %d\n", pinState);
	//buttonState = ( ( buttonState << 1 ) | pinState | 0xFE00 );	//sampling 8 times
	buttonState = ( ( buttonState << 1 ) | pinState | 0xE000 );	//sampling 12 times

	//if( buttonState == 0xFF00 )//for 8 samples
	if( buttonState == 0xF000 )	//for 12 samples
		return TRUE;

	return FALSE;
	
}


void *consumerThread_fn(void *arg)
{
   	int ret;
	int fd = (int)arg;
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 0};
	unsigned long led_ON_time = 0;
 
	while(1) {
		ret = sem_trywait(&sem_filledslots);
		if(ret == 0){	//success; queue is not empty
                	pthread_mutex_lock(&m1);
			led_ON_time = userspace_queue[q_out];
                	pthread_mutex_unlock(&m1); //release the mutex
			q_out = (q_out + 1) % USERSPACE_QUEUE_SIZE;			

			if(sem_post(&sem_freeslots) == -1) {
				perror("1.consumer");
				pthread_exit(NULL);
			}
		}
		else if(ret == -1 && errno != EAGAIN) {
			printf("ret = %d\n", errno);
			perror("2.consumer");
			pthread_exit(NULL);       
		}
                 
/*                pthread_mutex_lock(&m2);
		led_ON_time = next_ON_time;
                pthread_mutex_unlock(&m2); //release the mutex
*/
//		printf("led_ON_time = %lu\n", led_ON_time);
		if(led_ON_time > 0) {
			led_ON(fd);
			ts.tv_nsec = led_ON_time; 
                	ret = clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
                	if(ret)
                        	perror("consumer: clock_nanosleep");
		}
		led_OFF(fd);
		ts.tv_nsec = PWM_PERIOD_NANO_SEC - led_ON_time; 
                ret = clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
                if(ret)
                        perror("consumer: clock_nanosleep");
	}
}

void *getitem_Thread_fn(void *arg)
{
	unsigned int read_data;
	
	while(1){
		if(sem_wait(&sem_filledslots) == -1)
			pthread_exit(NULL);                        

                pthread_mutex_lock(&m1);
		read_data = userspace_queue[q_out];
                pthread_mutex_unlock(&m1); //release the mutex
		q_out = (q_out + 1) % USERSPACE_QUEUE_SIZE;			

		if(sem_post(&sem_freeslots) == -1)
			pthread_exit(NULL);
			
                pthread_mutex_lock(&m2);
		next_ON_time = read_data;
                pthread_mutex_unlock(&m2); //release the mutex
	}
}

void *producerThread_fn(void *arg)
{
   	int ret;
	int fd = (int)arg;
	unsigned int DutyCycle = 0;
	struct timespec ts;
	//struct timespec ts = {.tv_sec = 0, .tv_nsec = CLK_SWITCH_DEBOUNCE_PERIOD_NANO_SEC};

        ret = clock_gettime(CLOCK_REALTIME, &ts);
        //ret = clock_gettime(CLOCK_MONOTONIC, &ts);
        if(ret) {
                perror("clock_gettime");
                pthread_exit(NULL);
        }
        ts.tv_sec += CLK_REQ_PERIOD_SEC + ( ( ts.tv_nsec + CLK_SWITCH_DEBOUNCE_PERIOD_NANO_SEC ) / CLK_NANO_SEC_OVERFLOW );
        ts.tv_nsec = ( ts.tv_nsec + CLK_SWITCH_DEBOUNCE_PERIOD_NANO_SEC ) % CLK_NANO_SEC_OVERFLOW;

	while(1) {
		if(buttonDebounce(fd) == TRUE) {
			//printf("button press\n");
			DutyCycle += 1000000;			
			if(DutyCycle == DUTY_CYCLE_MAX)
    				DutyCycle = 0;
//			printf("DutyCycle = %u\n", DutyCycle);

			ret = sem_trywait(&sem_freeslots);
			if(ret == 0) {   //success; queue is not full
				//we are locking the mutex and entering the 
                        	//critical section !!!
                        	pthread_mutex_lock(&m1);
				userspace_queue[q_in] = DutyCycle;
                      		pthread_mutex_unlock(&m1); //release the mutex
                                                   //otherwise, other thread will be 
                                                   //blocked in the mutex
				q_in = (q_in + 1) % USERSPACE_QUEUE_SIZE;
				if(sem_post(&sem_filledslots) == -1) {
					perror("1.producer");
					pthread_exit(NULL);
				}
			}
			else if (ret == -1 && errno != EAGAIN ) {
				perror("2.producer");
				pthread_exit(NULL);            
			}            
				
/*			if(sem_post(&sem_buttonpress) == -1)
				pthread_exit(NULL);*/
		}

                ret = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, NULL);
                if(ret)
                        perror("3.clock_nanosleep");

                ts.tv_sec += CLK_REQ_PERIOD_SEC + ( ( ts.tv_nsec + CLK_SWITCH_DEBOUNCE_PERIOD_NANO_SEC ) / CLK_NANO_SEC_OVERFLOW );
                ts.tv_nsec = ( ts.tv_nsec + CLK_SWITCH_DEBOUNCE_PERIOD_NANO_SEC ) % CLK_NANO_SEC_OVERFLOW;


        /*        ret = clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
                if(ret)
                        perror("1.clock_nanosleep");*/
	}
}

void *putitem_Thread_fn(void *arg) 
{
	unsigned int DutyCycle = 0;
	
	while(1){
		if(sem_wait(&sem_buttonpress) == -1)
			pthread_exit(NULL); 

		DutyCycle += 1000000;			
		if(DutyCycle == DUTY_CYCLE_MAX)
    			DutyCycle = 0;
		printf("DutyCycle = %u\n", DutyCycle);
                //typical lightweight decrement operation 
		if(sem_wait(&sem_freeslots) == -1)
			pthread_exit(NULL);                        
		//we are locking the mutex and entering the 
                        //critical section !!!
                pthread_mutex_lock(&m1);
		userspace_queue[q_in] = DutyCycle;
               	pthread_mutex_unlock(&m1); //release the mutex
                                           //otherwise, other thread will be 
                                           //blocked in the mutex
		q_in = (q_in + 1) % USERSPACE_QUEUE_SIZE;
		//typical lightweight increment operation 
                if(sem_post(&sem_filledslots) == -1)
			pthread_exit(NULL);
	}

}

//user-space thread library ids - these are not same as 
//system space thread ids - do not mix up - however, 
//thread library APIs take care of hiding the abstraction !!!
//internally, these IDs may be mapped with system call APIs !!!

//user-space thread IDs are abstract and opaque !!!

pthread_t consumerThread_id, producerThread_id, putitem_Thread_id, getitem_Thread_id;
int main()
{

   	int ret;
   	struct sched_param param1;
        //mutexes also have attributes and attribute objects
        //most initialization rules are similar to that of thread creation !!!
        pthread_mutexattr_t ma1;

       	//int policy, s;

   	//these are abstract thread attr objects - 
   	//these can be local variables and
   	//they must be initialized before using them !!!!
	pthread_attr_t pthread_attr1, pthread_attr2;

	param1.sched_priority = 2;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param1);


/*        s = pthread_getschedparam(pthread_self(), &policy, &param1);
        printf("main thread    policy=%s, priority=%d\n",
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   param1.sched_priority);
*/
        //a mutex attribute object must be 
        //initialized using pthread_mutexattr_init() - 
        //this step of initialization is a must 
       	pthread_mutexattr_init(&ma1);//initialize to default attributes !!

       	//refer to manual page of pthread_mutexattr_settype() for 
        //different types of mutexes !!!

         //here, we are setting the type attribute of the 
         //mutex to NORMAL - we may change this, if needed
         //in the future, we may set other attributes of 
         //a mutex   
         pthread_mutexattr_settype(&ma1,PTHREAD_MUTEX_NORMAL);
        //pthread_mutexattr_settype(&ma1,PTHREAD_MUTEX_ERRORCHECK);
        //
        //to initialize a mutex object, we need to use
        //pthread_mutex_init() 
        //p1 is the ptr to the mutex object resident, in 
        //the user-space 
        //p2 is the ptr to a mutex attribute object, which 
        //must be initialized, as per the attributes needed, 
        //for p1

        //if p1 and p2 are correctly set-up and passed, 
        //pthread_mutex_init() will initialize the 
        //mutex, with appropriate attributes and the 
        //initial state is set to unlocked state 

        pthread_mutex_init(&m1,&ma1);
        pthread_mutex_init(&m2,&ma1);
        //p1 is the ptr to the semaphore object - 
        //in the case of thread semaphore/unnamed semaphore, 
        //the semaphore object is placed, in a global 
        //data segment shared among threads of the process
        //in addition, we directly deal , with the 
        //semaphore object and there is no key/id 
     

        //p2 is a flag, for private(private to a process) or 
        //pshared(shared between processes) semaphore object 
        //if p2 is set to 0, it will be treated, as a private 
        //semaphore object/counter 
        //if set to zero, the semaphore object must be resident, in 
        //a shared data segment, like data | heap | anonymous, 
        //not stack 
        //if p2 is set to 0, it must be used between threads
        //of the same process, not threads of different 
        //processes  
        //if p2 is set to non-zero, semaphore/counter will  be 
        //treated as a pshared semaphore - if p2 is set to 
        //non-zero, the semaphore object must be resident, in 
        //a shared memory region set-up, using shmget() or 
        //similar process shared memory mechanism 
        //p3 is the initial value of the semphore counter 
	if(sem_init(&sem_filledslots, 0, 0)){
		perror("error in sw_dboun_sem initialization");
		exit(1);
	}

	if(sem_init(&sem_freeslots, 0, USERSPACE_QUEUE_SIZE)){
		perror("error in sw_dboun_sem initialization");
		exit(1);
	}

	if(sem_init(&sem_buttonpress, 0, 0)){
		perror("error in sw_dboun_sem initialization");
		exit(1);
	}
        //a file/IO system call API
   	pdfd0  = open("/sys/bus/platform/devices/custom_leds/leds/led1", O_RDWR); 
   	//pdfd0  = open("/dev/pseudo0", O_RDWR|O_NONBLOCK); //device file accessed in non-blocking mode
   	if(pdfd0<0) { 
		perror("error in opening led1"); 
		exit(1);
	}
        //similar file/IO system call API
   	pdfd1  = open("/sys/bus/platform/devices/custom_switch/Switches/sw1", O_RDONLY); 
   	//pdfd0  = open("/dev/pseudo0", O_RDWR|O_NONBLOCK); //device file accessed in non-blocking mode
   	if(pdfd1<0) { 
		perror("error in opening switch"); 
		exit(1);
	}

        //another file/IO system call API 
        write(pdfd0, "0", 1); //turn off led1

      	//initialization of the thread attribute obj. is a must
      	//before using the attribute object in the pthread_create()

      	//after initializing the thread attr object, you can set the
      	//attributes as per your requirements 

      	//still, initializing thread attr object ensures that 
      	//all attributes are initially set to default system settings
      	//for more details on default settings, refer to manual pages !!!
      	ret = pthread_attr_init(&pthread_attr1);
      	if(ret>0) {
        	printf("error in the pthread_attr1 initialization\n");
      	}
	//apis below cannot be used to change non realtime priorities !!!

   	//setting the policy to realtime, priority based 
   	pthread_attr_setschedpolicy(&pthread_attr1, SCHED_FIFO);
   	//pthread_attr_setschedpolicy(&pthread_attr1, SCHED_RR);

   	//realtime priority of 1 ( 1-99 is the range) 
   	param1.sched_priority = 1;
   	pthread_attr_setschedparam(&pthread_attr1, &param1);

   	//you must set the following attribute, if you wish to use
   	//explicit scheduling parameters - if you do not, system 
   	//will ignore the explicit scheduling parameters passed
   	//via attrib object and inherit the scheduling parameters
   	//of the creating sibling thread !!! 
   	pthread_attr_setinheritsched(&pthread_attr1, PTHREAD_EXPLICIT_SCHED);

	ret = pthread_create(&consumerThread_id, &pthread_attr1, consumerThread_fn, (void*)pdfd0);
 	if(ret>0) { 
		printf("error in consumerThread creation\n"); 
		exit(4); 
	}   

	pthread_attr_destroy(&pthread_attr1);

	printf("1.debug point\n");
      	ret = pthread_attr_init(&pthread_attr2);
      	if(ret>0) {
        	printf("error in the pthread_attr2 initialization\n");
      	}

   	//setting the policy to realtime, priority based 
   	pthread_attr_setschedpolicy(&pthread_attr2, SCHED_FIFO);
   	//pthread_attr_setschedpolicy(&pthread_attr2, SCHED_RR);

   	//realtime priority of 1 ( 1-99 is the range) 
   	param1.sched_priority = 1;
   	pthread_attr_setschedparam(&pthread_attr2, &param1);

  	pthread_attr_setinheritsched(&pthread_attr2, PTHREAD_EXPLICIT_SCHED);

	printf("2.debug point\n");
  	ret = pthread_create(&producerThread_id, &pthread_attr1, producerThread_fn, (void*)pdfd1);
   	if(ret>0) { 
		printf("error in producerThread creation\n"); 
		exit(4); 
	}   

	pthread_attr_destroy(&pthread_attr2);

/*      	ret = pthread_attr_init(&pthread_attr1);
      	if(ret>0) {
        	printf("error in the pthread_attr1 initialization\n");
      	}
	//apis below cannot be used to change non realtime priorities !!!

   	//setting the policy to realtime, priority based 
   	pthread_attr_setschedpolicy(&pthread_attr1, SCHED_FIFO);
   	//pthread_attr_setschedpolicy(&pthread_attr1, SCHED_RR);

   	//realtime priority of 1 ( 1-99 is the range) 
   	param1.sched_priority = 1;
   	pthread_attr_setschedparam(&pthread_attr1, &param1);

   	//you must set the following attribute, if you wish to use
   	//explicit scheduling parameters - if you do not, system 
   	//will ignore the explicit scheduling parameters passed
   	//via attrib object and inherit the scheduling parameters
   	//of the creating sibling thread !!! 
   	pthread_attr_setinheritsched(&pthread_attr1, PTHREAD_EXPLICIT_SCHED);

	ret = pthread_create(&getitem_Thread_id, &pthread_attr1, getitem_Thread_fn, NULL);
 	if(ret>0) { 
		printf("error in consumerThread creation\n"); 
		exit(4); 
	}   

	pthread_attr_destroy(&pthread_attr1);

      	ret = pthread_attr_init(&pthread_attr1);
      	if(ret>0) {
        	printf("error in the pthread_attr1 initialization\n");
      	}
	//apis below cannot be used to change non realtime priorities !!!

   	//setting the policy to realtime, priority based 
   	pthread_attr_setschedpolicy(&pthread_attr1, SCHED_FIFO);
   	//pthread_attr_setschedpolicy(&pthread_attr1, SCHED_RR);

   	//realtime priority of 1 ( 1-99 is the range) 
   	param1.sched_priority = 1;
   	pthread_attr_setschedparam(&pthread_attr1, &param1);

   	//you must set the following attribute, if you wish to use
   	//explicit scheduling parameters - if you do not, system 
   	//will ignore the explicit scheduling parameters passed
   	//via attrib object and inherit the scheduling parameters
   	//of the creating sibling thread !!! 
   	pthread_attr_setinheritsched(&pthread_attr1, PTHREAD_EXPLICIT_SCHED);

	ret = pthread_create(&putitem_Thread_id, &pthread_attr1, putitem_Thread_fn, NULL);
 	if(ret>0) { 
		printf("error in consumerThread creation\n"); 
		exit(4); 
	}   

	pthread_attr_destroy(&pthread_attr1);*/
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
   	pthread_join(consumerThread_id,NULL);
   	pthread_join(producerThread_id,NULL);
   	//pthread_join(getitem_Thread_id,NULL);
   	//pthread_join(putitem_Thread_id,NULL);

   	exit(0);
}






