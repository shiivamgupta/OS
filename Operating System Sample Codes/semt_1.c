#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>

#define KEY1 1234

union semun {
             int val;                  /* value for SETVAL */
             struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
             unsigned short *array;    /* array for GETALL, SETALL */
                                       /* Linux specific part: */
             struct seminfo *__buf;    /* buffer for IPC_INFO */
       };


int main()

  {

    int ret1, ret2, id1, id2;
    union semun u1;
    struct sembuf sboa[2];
    unsigned short ary1[3],ary[3] = { 50,0,1};
    
    //KEY1 is the key value 
    //p2 is set to 1, so the number of semaphores will 
    //be 1
    //p3 is a set of flags - currently, we will be 
    //using  a default set of flags - for instance, 
   //there are certain multi-user credentials , 
    //that can be changed, using flags
    //if a new semaphore object is created/set-up,
    //the system will return id of the new semaphore 
    //object - we will be using this id, for further 
    //system call APIs/operations 
    //we will not use KEY, in most cases - this is the set-up  
    id1 = semget(KEY1, 1, IPC_CREAT|0666);
    //id1 = semget(KEY1, 3, IPC_CREAT|0600);
    //if the return value is <0, there is an error, 
    //in creating/setting-up a semaphore object/semaphores
    if(id1<0) { perror("error in semaphore creation"); exit(1); }    



    //what do we achieve by the end of the following 2 statements ???
    //first semaphore of the semaphore object is initialized to zero !!! 
    //error checking is missing in semctl() - add it ??
    u1.val = 1;
    //first parameter is the id of the semaphore object 
    //second parameter will be an index of a semaphore, 
    //in this semaphore object
    //third parameter will  be a command - 
    //can be SETVAL|GETVAL|SETALL|GETALL
    //in this context, second parameter is set to index 0 
    //cmd used is SETVAL - SETVAL is used to set the initial 
    //value of a single semaphore, in a semaphore object  
    //fourth parameter is an union, which contains a value or values, which 
    //are used to initialize one or more semaphores
    //for instance, when the cmd is SETVAL, union's val field is used to 
    //initialize the value of a semaphore, with index passed, in param2
    //this union and its fields are used, as per cmd field  
    //
    //different fields of the union are used, as per the cmd value 
    semctl(id1,0,SETVAL,u1);
    //if semctl() is successful, in this context, the initial value 
    //of index 0 semaphore is set to 1 
    
    //what do we achieve by the following 2 state ments ??
    //
    //
    //add error checking to semctl() !!
    //in this case, GETVAL is the command and param4 is unused 
    //instead the current value of the semaphore specified, in 
    //param2 will be returned, by semctl 
    ret1 = semctl(id1,0,GETVAL); 
    printf("1..before decrement..the value of sem 0 is %lu\n",ret1);
   

    /*u1.val = 50;

    semctl(id1,0,SETVAL,u1); 
    u1.val = 0;
    semctl(id1,1,SETVAL,u1); 
    u1.val = 1;
    semctl(id1,2,SETVAL,u1); 
     

    ret1 = semctl(id1,0,GETVAL); 
    printf("the value of sem 0 is %lu\n",ret1); 
    ret1 = semctl(id1,1,GETVAL); 
    printf("the value of sem 1 is %lu\n",ret1);
    ret1 = semctl(id1,2,GETVAL); 
    printf("the value of sem 2 is %lu\n",ret1);*/

    //either above or below
    //u1.array = ary;
    
    //semctl(id1,0,SETALL,u1);

    //u1.array = ary1;

    //semctl(id1,0,GETALL,u1);

    //printf("the values are %lu .. %lu .. %lu\n", ary1[0],
    //	   ary1[1],ary1[2]);

     //operate on 0
    //first field, in the object is index no. of a semaphore
    //second field, in the object is dec or inc operation (-1 or 
                                                           +1)  
    //third field is flags - in this case, default is 0                 
    sboa[0].sem_num = 0;  //we are operating on the first semaphore
    sboa[0].sem_op = -1;  //the operation is decrement operation 
    sboa[0].sem_flg = 0;  //normally, flags can be set to 0 

    //p2 is just pointer to a parameter object used, for 
    //passing certain parameters - this object is not 
    //a semaphore object - semaphore object is , in system-space
    //p3 is related to p2 - refer to text file, for comments 
    //in this context, semop() will decrement semaphore index 0, 
    //as per semaphore rules  
    semop(id1, sboa, 1); 

    //operate on 0
    //first field, in the object is index no. of a semaphore
    //second field, in the object is dec or inc operation (-1 or 
                                                           +1)  
    //third field is flags - in this case, default is 0                 
    sboa[0].sem_num = 0;  //we are operating on the first semaphore
    sboa[0].sem_op = +1;  //the operation is decrement operation 
    sboa[0].sem_flg = 0;  //normally, flags can be set to 0 

    //p2 is just pointer to a parameter object used, for 
    //passing certain parameters - this object is not 
    //a semaphore object - semaphore object is , in system-space
    //p3 is related to p2 - refer to text file, for comments 
    //in this context, semop() will increment semaphore index 0, 
    //as per semaphore rules  
    semop(id1, sboa, 1);
    ret1 = semctl(id1,0,GETVAL); 
    printf("2..after decrement..the value of sem 0 is %lu\n",ret1);


    //operate on 0 and 2 
    //sboa[0].sem_num = 0;
    //sboa[0].sem_op = -1;
    //sboa[0].sem_flg = 0;

    //sboa[1].sem_num = 2;
    //sboa[1].sem_op = -1;
    //sboa[1].sem_flg = 0;


    //semop(id1, sboa, 2); 

    /*semctl(id1,0,GETALL,u1);

    printf("the values are %lu .. %lu .. %lu\n", ary1[0],
	   ary1[1],ary1[2]);

    //operate on 0 and 2 
    sboa[0].sem_num = 1;
    sboa[0].sem_op = -1;
    sboa[0].sem_flg = 0;

    sboa[1].sem_num = 2;
    sboa[1].sem_op = -1;
    sboa[1].sem_flg = 0;


    semop(id1, sboa, 2);

    semctl(id1,0,GETALL,u1);*/

    printf("the values are %lu .. %lu .. %lu\n", ary1[0],
	   ary1[1],ary1[2]);

   //in this context, second parameter is ignored -
   //meaning, when we delete /destroy a semaphore, 
   //all semaphores of the semaphore object and the
   //semaphore object are destroyed !!!!
   //we cannot delete a single semaphore 
   //in addition, for this context, p2 is ignored - 
   //p2 normally represents semaphore instance/index,
   //but in this context, it is ignored  

    semctl(id1,0,IPC_RMID); //to delete or destroy the 
                            // semaphore object and all
                            // all semaphores in it 
   

}

