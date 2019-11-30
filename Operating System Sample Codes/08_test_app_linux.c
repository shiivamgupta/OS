/* date:feb-1 2013 */
/*final user-space code. */

#define __GNU_SOURCE

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<linux/ioctl.h>
#include<sched.h>
#include<time.h>
#include<string.h>

#define N 5
#define MAJOR 251 

#define MYIOCTL_TYPE 'k'

#define IOC_RESET   _IO(MY_MAGIC,0)
#define IOC_READ    _IOR(MY_MAGIC,1,int)
#define W_8255_1CR  _IOW(MYIOCTL_TYPE,3,int)
#define W_8255_1A   _IOW(MYIOCTL_TYPE,4,int)
#define W_8255_1B   _IOW(MYIOCTL_TYPE,5,int)
#define W_8255_1C   _IOW(MYIOCTL_TYPE,6,int)
#define R_8255_1A   _IOR(MYIOCTL_TYPE,7,int)
#define R_8255_1B   _IOR(MYIOCTL_TYPE,8,int)
#define R_8255_1C   _IOR(MYIOCTL_TYPE,9,int)

#define W_8255_2CR  _IOW(MYIOCTL_TYPE,10,int)
#define W_8255_2A   _IOW(MYIOCTL_TYPE,11,int)
#define W_8255_2B   _IOW(MYIOCTL_TYPE,12,int)
#define W_8255_2C   _IOW(MYIOCTL_TYPE,13,int)
#define R_8255_2A   _IOR(MYIOCTL_TYPE,14,int)
#define R_8255_2B   _IOR(MYIOCTL_TYPE,15,int)
#define R_8255_2C   _IOR(MYIOCTL_TYPE,16,int)

#define W_8254_CR   _IOW(MYIOCTL_TYPE,17,int)
#define LATCH_T     _IOW(MYIOCTL_TYPE,18,int)

#define W_T0  	  _IOW(MYIOCTL_TYPE,19,int)
#define W_T1  	  _IOW(MYIOCTL_TYPE,20,int)
#define W_T2  	  _IOW(MYIOCTL_TYPE,21,int)
#define R_T0  	  _IOR(MYIOCTL_TYPE,22,int)
#define R_T1  	  _IOR(MYIOCTL_TYPE,23,int)
#define R_T2  	  _IOR(MYIOCTL_TYPE,24,int)

#define outport_byte _IOW(MYIOCTL_TYPE,25,int)
#define inport_byte  _IOR(MYIOCTL_TYPE,26,int)

#define INT_LATENCY     _IOR(MYIOCTL_TYPE,27,int)  //for log transfer
#define CTX_SWITCH      _IOR(MYIOCTL_TYPE,28,int)  //for log transfer
#define MODE_SWITCH      _IOR(MYIOCTL_TYPE,29,int)  //for log transfer

/* macros for setting the mode for 8255 */



void *pthread_read(void *arg)  //your Test Bench
{
	int fd = *(int *)arg,ret,sam=0,ctx_file,mode_file;
	unsigned char argv,ctx_buff[30],mode_buff[30];
	long mode_nsec;
	long ctx_switch,mode_switch;
	struct timespec mode;

	ret=ioctl(fd,W_8255_2CR,0xb0);
	ctx_file=open("./ctx.dat",O_RDWR);
	mode_file=open("./mode.dat",O_RDWR);
	while(sam++<10)//change this for changing the number of samples that you want to collect for benchmarking.
	{
//		printf("Right before reading port A --- BLOCKING\n");
		ret=ioctl(fd,R_8255_2A,&argv);
		clock_gettime(CLOCK_REALTIME,&mode);//second time stamp for mode switch time.First timestamp is 						     taken in driver,the diff is calculated here in the user-space.

		ret=ioctl(fd,CTX_SWITCH,&ctx_switch);  //for retreiving the context switch time.By this time it is                                                         already created in driver.

		sprintf(ctx_buff,"%d	%ld\n",sam,ctx_switch);
		write(ctx_file,ctx_buff,strlen(ctx_buff));

		ret=ioctl(fd,MODE_SWITCH,&mode_nsec);
		mode_switch=mode.tv_nsec - mode_nsec;
		sprintf(mode_buff,"%d	%ld\n",sam,mode_switch);
		write(mode_file,mode_buff,strlen(mode_buff));

//		printf("Right after reading port A and value is %d --- WAKEUP\n",argv);
//		printf("Writing onto Port B of 8255_1\n");
		ret = ioctl(fd,W_8255_2B,&argv);
	}
	close(ctx_file);
	close(mode_file);
	pthread_exit(NULL);
}
void *pthread_write(void *arg)  //your Measuring Bench
{
	int fd1,sam=0,fd2,cnt=0;
	long min=0,max=0,temp;
	long log_buff;
	char log[30];
	struct timespec t1,t2;
	long diff_sec,diff_nsec;
	int fd = (int )arg,ret,count;
	unsigned char argv=22,read,buf[30];
	fd1=open("./int_response.dat",O_RDWR);
	if(fd1)
		printf("file open\n");
	
	fd2 = open("./int_latency.dat",O_RDWR);
	if(fd2)
		printf("fd2 opened\n");
	
	ret=ioctl(fd,W_8255_1CR,0xa2);
//	printf("Writing onto Port A of 8255_1\n");
	while(sam++<10)
	{
		ret=ioctl(fd,W_8255_1A,&argv);
//		clock_gettime(CLOCK_REALTIME,&t1);  
//		printf("Right before reading port B --- BLOCKING\n");
		ret=ioctl(fd,R_8255_1B,&read);
//		printf("Right after reading port B and value is %d --- WAKEUP\n",read);
//		clock_gettime(CLOCK_REALTIME,&t2);
	
		ret = ioctl(fd,INT_LATENCY,&log_buff);
		printf("log : %ld\n",log_buff);
		if(sam==1)
			min=log_buff;
		if(log_buff>10000)
			cnt++;
		min=min<log_buff?min:log_buff;
		max=max>log_buff?max:log_buff;
		sprintf(log,"%d	%ld\n",sam,log_buff);
		write(fd2,log,strlen(log));

		diff_nsec = t2.tv_nsec - t1.tv_nsec;
		diff_sec = t2.tv_sec - t1.tv_sec;
		sprintf(buf,"secs:%ld,nsecs:%ld\n",diff_sec,diff_nsec);
		write(fd1,buf,strlen(buf));
		write(0,log_buff,sizeof(buf));
		count++;
	}
	printf("samples collected:%d\n",count);
	printf("max : %d  ,  min : %d\n",max,min);
	printf("cnt : %d\n",cnt);
	close(fd1);
	close(fd2);
	pthread_exit(NULL);
}

pthread_attr_t at1,at2;
struct sched_param p1,p2;

	pthread_t thread1,thread2;
int main()
{
	int fd,i,arg;
	char buff[20]={0};
	
        //we must initialize a thread attribute object
        //before setting specific attributes, in the 
        //object - as per rules of thread APIs  
        pthread_attr_init(&at1); 
	pthread_attr_init(&at2);
	p1.sched_priority=90;
	p2.sched_priority=80;
        //using an API, we are setting the scheduling 
        //policy attribute of a thread attribute  object  
	ret = pthread_attr_setschedpolicy(&at1,SCHED_FIFO);
        //check for errors, at every API 
        if(ret>0) { printf("error in pthread_attr_setschedpolicy\n"); 
                    exit(2); }
        //setting the scheduling priority attribute 
        //of an attribute object
	pthread_attr_setschedparam(&at1,&p1);
        //setting the explicit scheduling 
        //attribute of an attribute object - if this 
        //attribute is not set,the attributes of the 
        //attributes object will be ignored - these are
        //very specific rules   
	pthread_attr_setinheritsched(&at1,PTHREAD_EXPLICIT_SCHED);

	pthread_attr_setschedpolicy(&at2,SCHED_FIFO);
	pthread_attr_setschedparam(&at2,&p2);
	pthread_attr_setinheritsched(&at2,PTHREAD_EXPLICIT_SCHED);

        //with the thread attributes' objects set-up, 
        //we can pass pointers to these objects to 
        //pthread_create() 

	for(i=0;i<1;i++)
	{
		sprintf(buff,"/dev/pcidiot_dev1");
		fd=open(buff,O_RDWR);
		
		sprintf(buff,"/dev/pcidiot_dev2");
		fd1=open(buff,O_RDWR);
          //in this thread, we are also setting the scheduling 
         //policy and parameters, using a scheduling attribute object
         //the following threads will be created, with 
         //explicit scheduling policy and other 
         //parameters, as per settings 
     //     pthread_create(&thread1,&at1,pthread_read,(void*)fd);
    //	  pthread_create(&thread1,&at2,pthread_read,(void*)fd);
	  
          //pthread_attr_setschedpolicy(&at2,SCHED_FIFO);
	  //pthread_attr_setschedparam(&at2,&p2);
	  //pthread_attr_setinheritsched(&at2,PTHREAD_EXPLICIT_SCHED);
     //in this context, we are passing handle of an IO device
     //instance to param4 - this handle will be passed to thread method
     //in the thread method, this handle will be used to 
     //access the IO device instance
     //in many scenarios, there will be several IO device instances
     //and we will need to pass different handles to different 
     //threads/thread methods to access appropriate 
     //IO device instances  
          pthread_create(&thread1,&at1,pthread_read,&fd);
	  pthread_create(&thread2,&at2,pthread_write,&fd);
	}
		pthread_join(thread1,NULL);
		pthread_join(thread2,NULL);

}
