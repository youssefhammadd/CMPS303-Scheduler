#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

typedef short bool;
#define true 1
#define false 0


#define msgQueue 10
#define SHKEY 300
#define SHKEY2 400
#define SHKEY3 500
#define SHKEY4 600


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}


//============================================ struct for a process ==========================================
/*each process has it's id , parents id ,it's state if it is  RUNNING or it is WAITING or it is FINISHED , running time , arrival time
, priority ,turnaround time ,remaining time ,waiting time , finish time and weighted turnaround time */
struct Process
{
    int ID;//process id
    int ParentID;//parent id
    int ArrivalTime;
    int FixedTime;
    int RunTime;
    int Priority;
    int TurnAroundTime;
    int RemainingTime;
    int WaitingTime;
    float WeightedTurnAround;
    int FinishTime;
    int State;
    char state[20];//store if the process is RUNNING or it is WAITING or it is FINISHED
    int pid;

};
//===============================================================================================================
//======================================== struct to be sended throught msg queue ===============================
struct msgbuff
{
    long mtype;
    struct Process Process;
};
//===============================================================================================================

//======================================== enum to store the scheduler algorithm to be perform ==================
enum SchedulerALgo
{
    hpf = 1, //uses priority queue and it's priority in enqueue is the priority of the process
    srtn = 2,//uses priority queue and it's priority in enqueue is the remaining time of the process
    rr = 3,  //uses a normal queue
};
//===============================================================================================================

//======================================= starting to implement our data structure ==============================

//======================================= struct to Node ========================================================
//first we need to have a node that store address of the process and has a pointer to the next node
struct Node
{
    struct Process* P;
    struct Node* next;
};

//===============================================================================================================



//======================================= function to create a new node =========================================
struct Node* createNode(int id , int arrivaltime, int fixedtime, int runtime, int remainingtime , int priority , int PID,int s, int pid)
{
   // printf("i entered the function createNode..\n");
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));//allocate memory dynamically
    //printf("i created new node using malloc\n");
    struct Process* newProcess = (struct Process*)malloc(sizeof(struct Process));
   // printf("i created new node using malloc\n");
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newProcess->ID=id;
    newProcess->ArrivalTime = arrivaltime;
    newProcess->FixedTime = fixedtime;
    newProcess->RunTime = runtime;
    newProcess->RemainingTime=remainingtime;
    newProcess->Priority=priority;
    newProcess->ParentID=PID;
    newProcess->State=s;
    newProcess->pid=pid;
    strcpy(newProcess->state,"WAITING");
    newProcess->WaitingTime=0;

    newNode->P=newProcess;

    newNode->next = NULL;
   // printf("the created node will be returned now\n");
    return newNode;
}
//don't forget to use free(ptr_obj) , and the ptr_obj is the pointer to object that created dynamically to free the memory
//================================================================================================================

//======================= struct for the queue (modified work as Priority and normal queue) ======================
struct Queue
{
    struct Node *head;
    struct Node *tail;
    int count;
};
//================================================================================================================

//======================================= function to create a new queue =========================================
//this function creates a queue and set return it
struct Queue* createQueue()
{
    struct Queue* newQueue = (struct Queue*)malloc(sizeof(struct Queue));
    if (newQueue == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newQueue->head=NULL;
    newQueue->tail=NULL;
    newQueue->count=0;
    return newQueue;
}
//don't forget to use free(ptr_obj) , and the ptr_obj is the pointer to object that created dynamically to free the memory
//================================================================================================================

//=============================== function to check if the queue is empty =========================================
bool isEmpty(struct Queue** Q)
{
    if((*Q)->head == NULL)
        return true;
    else
        return false;
}
//================================================================================================================

//========================= function to Peek the first element in the queue ======================================
struct Process* PeekFront(struct Queue** Q)
{
    if(isEmpty(Q))
    {
        return NULL;
    }
    else
    {
        struct Process * P = (*Q)->head->P;//get from the queue the first node and from this node get the process
        return P;
    }
}
//================================================================================================================

//============== function dequeue delete the first node in the queue and return the process in that node ======================
struct Process * Dequeue(struct Queue** Q,enum SchedulerALgo i)
{
   
   // printf("i entered the Dequeue function..\n");
    //printf("%p\n",((*Q)->head)->next);
    //printf("%p\n",((*Q)->tail)->next);
    if(isEmpty(Q))
    {
        printf("Queue is empty...\n");
        return NULL;

    }
    //printf("i got the process at the head which is..\n");
    struct Process* P;
    P=((*Q)->head)->P;//i get the process in the first node in the queue and P is the return address
    //printProcess(P);


    //check if it is the only element in the queue
    struct Node* NodetobeDeleted=((*Q)->head);
    if((*Q)->count==1)//there is only 1 element in the queue then remove it and let the head equal null
    {
        //free(NodetobeDeleted);//i freed the node that the head points to

        ((*Q)->tail)=NULL;

        //free(NodetobeDeleted);//i freed the node that the head points to

        ((*Q)->head)=NULL;

        ((*Q)->count)=((*Q)->count)-1;
        

        return P;
    }
    else
    {
        //printf("there is more then one element in the queue..\n");
        ((*Q)->head)=((*Q)->head)->next;//update the head to be the next node
        if(i==3)
        {
            ((*Q)->tail)->next = ((*Q)->head);//update the tail to the new head
        }
        NodetobeDeleted->next=NULL;
        free(NodetobeDeleted);
        //printf("i freed the head and apply to the head the next node..\n");
        
        (*Q)->count--;
        return P;
    }

}
//================================================================================================================

//============== function enqueue to enqueue the node in the suitable place in the queue according to it's scheduler algorithm ======================
void Enqueue(struct Queue** Q,struct Process ProcessObj,enum SchedulerALgo i)
{
    //printf("i entered the function enqueue..\n");
    struct Node* CreatedNode = createNode(ProcessObj.ID,ProcessObj.ArrivalTime,ProcessObj.FixedTime,ProcessObj.RunTime,ProcessObj.RemainingTime,ProcessObj.Priority,ProcessObj.ParentID,ProcessObj.State,ProcessObj.pid);

    //handle the case if the queue is empty
    //printf("i entered the function enqueue..\n");

    if(isEmpty(Q))//adding the node in a empty queue
    {
      //  printf("i entered the conditin if the queue empty..\n");
        if(i==3)
        {
            
            ((*Q)->head) = CreatedNode;
            ((*Q)->tail) = CreatedNode;
            CreatedNode->next=CreatedNode;//let the node point to itself
            (*Q)->count++;
        }
        else
        {
            //printf("i entered the to add an object to an empty queue using HPF or SRTN..\n");
            
            ((*Q)->head) = CreatedNode;
            ((*Q)->tail) = CreatedNode;
            (*Q)->count++;
        }
        return;
    }

    struct Node* Temp=((*Q)->head);
    switch(i)
    {
        case hpf:
        //printf("i entered to enqueue a process to HPF..\n");
            if (((*Q)->head)->P->Priority > ProcessObj.Priority)
            {//if the priority of the process entering the queue is higher than the priority of the first process in the queue then enqueue the new process at the head
                CreatedNode->next = ((*Q)->head);
                ((*Q)->head) = CreatedNode;
            }
            else
            {
                //move untill you found a node that has a process with the lower priority than the entering process and then added this node before it
                while (Temp->next != NULL && Temp->next->P->Priority <= ProcessObj.Priority)
                {
                    Temp = Temp->next;
                }
                CreatedNode->next = Temp->next;
                Temp->next = CreatedNode;
            }
            (*Q)->count=(*Q)->count+1;
            break;
        case srtn:
        //compare according to the remaining time of the entering process
        //printf("i entered to enqueue a process to SRTN..\n");
            if (((*Q)->head)->P->RemainingTime > ProcessObj.RemainingTime)
            {
                CreatedNode->next = ((*Q)->head);
                ((*Q)->head) = CreatedNode;
                //if the entering process has remaining time shorter than the process at the head so let the entering process be the head
            }
            else
            {
                while (Temp->next != NULL && Temp->next->P->RemainingTime <= ProcessObj.RemainingTime)
                {
                    Temp = Temp->next;
                    //move in the queue until finding a process that has a larger remaing time
                }
                CreatedNode->next = Temp->next;
                Temp->next = CreatedNode;
            }
            (*Q)->count=(*Q)->count+1;
            break;
        case rr://as a circular queue
        //printf("i entered to enqueue a process to RR..\n");
            CreatedNode->next=(*Q)->head;
            (*Q)->tail->next=CreatedNode;
            (*Q)->tail=CreatedNode;

            (*Q)->count=(*Q)->count+1;
            break;
    }

}

//================================================================================================================


void printProcess(struct Process* p)
{
   printf("id is: %d\n", p->ID);
  // printf("pID is: %d\n", p->ParentID);
   printf("arrival times is: %d\n", p->ArrivalTime);
 //  printf("runTime is: %d\n", p->RunTime);
   printf("prioriry is: %d\n", p->Priority);
 //  printf("state is: %s\n", p->state);
   printf("remaining time is: %d\n", p->RemainingTime);
/*  printf("finish time is: %d\n", p->FinishTime);
   printf("Waiting time is: %d\n", p->WaitingTime);
   printf("TA is: %d\n", p->TurnAroundTime);
   printf("WTA is: %f\n", p->WeightedTurnAround);*/
}

void printList(struct Process Proccesses[], int inputProccessesCount)
{
    for(int i=0; i<inputProccessesCount; i++)
    {
        printf("\nProccess '%d, %d, %d, %d'\n", Proccesses[i].ID, Proccesses[i].ArrivalTime, Proccesses[i].RunTime, Proccesses[i].Priority);
    }
}

void printQueue(struct Queue **q)
{
   if (isEmpty(q))
    {
        printf("\nEmpty Queue\n");
        return;
    }
   printf("\nStart of the Q, Count: %d\n", (*q)->count);
   struct Node* start = ((*q)->head);
   printProcess(start->P);
   int counter=((*q)->count) ;
   //counter =counter-1;
   while (start->next != NULL)
   {
    counter--;
        if(counter == 0){
            break;
        }
      printProcess(start->next->P);
      start = start->next;
      
     // printf("%d\n",counter);
      //sleep(2);

   }
   printf("\nEND of the Q\n");
}