#include "headers.h"

bool stillSending = true;
void handler(int);
int processes_count=0;


void HPF( struct Queue** Ready_Processes);
void SRTN( struct Queue** Ready_Processes);
void RR( struct Queue** Ready_Processes,int quantum);

void start(struct Process* process);
void finish(struct Process process);
void resume(struct Process process);
void Pause(struct Process process);

FILE *outputFile1;
FILE *outputFile2;

double totalWTA = 0;
double totalWait = 0;
double totalRun = 0;
int totalSquareWTA = 0;
double* perProcessWTA;
int currentProcessIndex = 0;
int finished_process_count=0,shmid,msgid,RecievingQueue;
int Algo;
struct  msgbuff msgProcess;

int* allProcessCount;
char* ProcessPath;

int main(int argc, char * argv[])
{
    initClk();
    printf("entered the scheduler::\n");
//========================================================================================================================
//Get the process path
    char processBuffer[500];
    getcwd(processBuffer, sizeof(processBuffer));
    ProcessPath = strcat(processBuffer, "/process.out");

//===================================== Readying command-line arguments  =================================================
    int type = atoi(argv[1]);                                               //1 for HPF , 2 for SRTN , 3 for RR
    Algo=type;
    printf("algo type: %d\n",Algo);
    int quantum = atoi(argv[2]);                                            //RR quantum    -1 if HPF or SRTN
    // argv[3] is the parent pid
    processes_count = atoi(argv[4]);                                          // argv[4] is the number of processes will come to the scheduler
     printf("number of processes: %d\n",processes_count);
//========================================================================================================================

//===================================== Initializing output files  =======================================================
    outputFile1 = fopen("scheduler.log", "w");
    outputFile2 = fopen("scheduler.perf", "w");
    fprintf(outputFile1, "#At time x process y state arr w total z remain y wait k\n");
//========================================================================================================================

        key_t key=ftok("keyfile",msgQueue);
        RecievingQueue=msgget(key,0666| IPC_CREAT);//created the queue that will recieve the process through

        int id_rows= shmget(SHKEY4, 4, IPC_CREAT | 0644);
        allProcessCount = (int *) shmat(id_rows, (void *)0, 0);//get the number processes that the scheduler will deal with using shared memory



        struct Queue* Ready_Processes = createQueue();//queue to add the processes that thier arrival time has came 
        int temp = *allProcessCount;
        perProcessWTA = malloc(sizeof(int) * temp);
        signal(SIGUSR1, handler);

//===================================== Choose algorithm type ============================================================
    if (type == 1) {
        printf("HPF Algorithm starts\n");
        HPF(&Ready_Processes);
        printf("HPF Algorithm ends\n");
    } else if (type == 2) {
        while(getClk()<1){
            
        }
        printf("SRTN Algorithm starts\n");
        SRTN(&Ready_Processes);
        printf("SRTN Algorithm ends\n");
        kill(getppid(),SIGINT);
    } else if (type == 3) {
        while(getClk()<1){
            
        }
        printf("RR Algorithm starts\n");
        RR(&Ready_Processes,quantum);
        printf("RR Algorithm ends\n");
        
    } else {
        printf("Invalid Algorithm type \n");
    }

    float utilization = ((float)totalRun / getClk()) * 100.0;
    float avgWTA = totalWTA / finished_process_count;
    float avgWait = totalWait / finished_process_count;
    
    float totalStdWTA = 0;
    for (int i = 0; i < finished_process_count; i++)
    {
        totalStdWTA += (perProcessWTA[i] - avgWTA) * (perProcessWTA[i] - avgWTA);
    }
    
    float stdWTA = sqrt((float)(totalStdWTA / finished_process_count));
    //stdWTA = totalSquareWTA / finished_process_count;

    fprintf(outputFile2, "CPU utilization = %.2f %% \n", utilization);

    fprintf(outputFile2, "Avg WTA = %.2f\n", avgWTA);
    fprintf(outputFile2, "Avg Waiting = %.2f\n", avgWait);
   fprintf(outputFile2, "Std WTA = %.2f\n", stdWTA);


    fclose(outputFile1);
    fclose(outputFile2);
    kill(atoi(argv[3]),SIGINT);
    
    //sleep(20);

   
    printf("scheduler main is about to end\n");
    destroyClk(true); //upon termination release the clock resources.
}
//========================================================================================================================

//============================================= Start Function ===========================================================
void start(struct Process* process) {
    if(process->ID <= -1 || process->ID > processes_count) {
        return;
    }
    fprintf(outputFile1, "At time %d process %d started, arrival time %d total %d remain %d wait %d\n",
            getClk(), process->ID, process->ArrivalTime, process->FixedTime,
            process->RemainingTime, getClk() - process->ArrivalTime);
    //printf("at Time : %d function start has recieved process with ID : %d ...\n",getClk(),process->ID);

    int Pid = fork();
    process->pid= Pid;
    if (process->pid == 0)
    { 
        //this part is for the child process
        /* printf("At time %d process with ID %d started, arrived at %d total %d remain %d wait %d\n",
            getClk(), process->ID, process->ArrivalTime, process->FixedTime,
            process->RemainingTime, getClk() - process->ArrivalTime);*/

        //we need to send the remaining time to the generated process
        char Remaining_Time[20];
        sprintf(Remaining_Time,"%d",process->RemainingTime);//convert the remaining time into string to be sended to the created process
                
        execl(ProcessPath,"process.out",Remaining_Time,NULL);  
    }
    else
    {
        printf("At time %d process with ID %d started, arrived at %d total %d remain %d wait %d\n",
            getClk(), process->ID, process->ArrivalTime, process->FixedTime,
            process->RemainingTime, getClk() - process->ArrivalTime);
        return; //return the start time of the arrived process
    }

}
//========================================================================================================================

//============================================= Finish Function ==========================================================
void finish(struct Process process) {
    if(process.ID == -1) {
        return;
    }
    int finishTime = getClk();
    int TA = finishTime - process.ArrivalTime;
    double WTA =(double) TA / process.FixedTime;
    int wait = TA - process.FixedTime;
    //sleep(1);
    printf("At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process.ID, process.ArrivalTime, process.FixedTime, wait, TA, WTA);
    fprintf(outputFile1, "At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process.ID, process.ArrivalTime, process.FixedTime, wait, TA, WTA);

    perProcessWTA[currentProcessIndex] = WTA;
    currentProcessIndex++;
    totalSquareWTA += WTA * WTA;
    totalWTA += WTA;
    totalWait += wait;
    totalRun += process.FixedTime;
    finished_process_count++;
}

//========================================================================================================================

//============================================= Resume Function ==========================================================
void resume(struct Process process)
{
    int currentTime = getClk();
    kill(process.pid, SIGCONT);
    fprintf(outputFile1, "At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
    currentTime, process.ID, process.ArrivalTime, process.FixedTime, process.RemainingTime,
    currentTime - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
    printf("At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
    currentTime, process.ID, process.ArrivalTime, process.FixedTime, process.RemainingTime,
    currentTime - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
}
//========================================================================================================================

//============================================= Pause Function ===========================================================
void Pause(struct Process process)
{
    kill(process.pid , SIGSTOP);
    fprintf(outputFile1, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
    getClk(), process.ID, process.ArrivalTime, process.FixedTime,process.RemainingTime,
    getClk() - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
    printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n",
    getClk(), process.ID, process.ArrivalTime, process.FixedTime,process.RemainingTime,
    getClk() - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
}
//========================================================================================================================

//============================================= HPF algorithm ============================================================
void HPF( struct Queue** Ready_Processes) {
    struct Process *process = NULL;
    int ID;
    int checkScheduler=-1;//if this variable is equal to -1 this means that the CPU is not processing any process so he can start a process at any instance
    int workingtime=0;
    int processes_counter=0;
    int starttime=-1;
    int currenttime=0;
   
        currenttime=getClk();
        msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0,!IPC_NOWAIT);
        Enqueue(Ready_Processes,msgProcess.Process,hpf);
        printf("the msg Queue has received a process at Time: %d with ID: %d \n",getClk(),msgProcess.Process.ID);
        printf("before entering the while loop\n");

            while(stillSending || (processes_counter-processes_count)!=0 ||checkScheduler==-1)
            {
                    //printf("before entering the 2nd while loop\n");
                while( msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0,IPC_NOWAIT) != -1)
                {   
                    printf("the msg Queue has received a process at Time: %d with ID: %d \n",getClk(),msgProcess.Process.ID);
                    if(msgProcess.Process.ID<=0 || msgProcess.Process.ID > processes_count+1)
                    {
                        break;
                    }
                   // printf("mtype %ld \n",msgProcess.mtype);
                    if(msgProcess.mtype==1)//the arrived message is garbage so check if there any process in the ready queue
                    {

                    //if(msgProcess.Process.ID==-1)
                      //  return;
                    //printf("the msg Queue has received a process at Time: %d with ID: %d \n",getClk(),msgProcess.Process.ID);
                    printf("message with id 5 is printed\n");
                    Enqueue(Ready_Processes,msgProcess.Process,hpf);
                    }
                    else
                        break;
                }

                if(!isEmpty(Ready_Processes) )
                {
                    //printProcess(PeekFront(Ready_Processes));
                    if(checkScheduler==-1 && PeekFront(Ready_Processes)->ArrivalTime <= getClk())
                    {
                        printProcess(PeekFront(Ready_Processes));
                     process=Dequeue(Ready_Processes,hpf); //get the process with the highest priority in the ready queue
                     printProcess(process);
                     ID = process->ID;
                     if(process->ID==-1)
                     return;
                     if(process == NULL)
                     {
                        return;
                     }
                     //there is a process has started 

                        printf("Process with ID : %d has arrived to the scheduler at time : %d and it is going to start now....\n",ID,getClk());
                        process->FixedTime=process->RunTime;
                        start(process);
                        starttime=getClk();//send the process to start which fork a process child
                        checkScheduler = 1;
                        
                        printf("The start time is : %d\n",starttime);
                        //printf("The Process counter is : %d\n",processes_counter);

                    }

                }
                //printf("%d\n\n",starttime + (process->RemainingTime));
                if(starttime + (process->RemainingTime) == getClk() && checkScheduler == 1)//this is the time the process will finish in
                {   
                    processes_counter++;
                    finish(*process);
                    checkScheduler = -1; //the cpu is now free to accept ready process
                    if(processes_count==processes_counter)
                    {
                        break;
                    }
                }
                else{
                //printf("number of seconds sleeped : %d\n", starttime + (process->RemainingTime) - getClk() - 1);
                    //sleep(starttime + (process->RemainingTime) - getClk() - 1);
                }

            //if(msgProcess.Process.ID == -1 && checkScheduler== -1){
            //sleep(50);
                //break;
                //process = NULL;
            //}

        }
                
    

}



//========================================================================================================================

//============================================= SRTN algorithm ============================================================

void SRTN(struct Queue** Ready_Processes) {
     struct Process *process,*temp=NULL;
     int ID;
     while(true) {
        if(!stillSending && isEmpty(Ready_Processes)) 
                return;
         if(!isEmpty(Ready_Processes) )
         {
             process=Dequeue(Ready_Processes,srtn);
             
             if(temp!=NULL)
              {
                 if(temp != NULL && temp->ID != process->ID && temp->RemainingTime!=0 )
                 {
                    Pause(*temp);
                 }
              }

             if (process->State==0) {
                process->State=1;
                process->FixedTime=process->RunTime;
                start(process);
                 
                sleep(1);
                process->RemainingTime=process->RemainingTime -1;
                temp=process;
            }
            else if(temp->ID != process->ID  ){
                 resume(*process);
                
                sleep(1);
                process->RemainingTime=process->RemainingTime -1;
                temp=process;
             }
             else{
                sleep(1);
                process->RemainingTime=process->RemainingTime -1;
             }
             if(process->RemainingTime==0)
             {
                finish(*process);
                temp=process;
            }
            else{
                
                     Enqueue(Ready_Processes,*process,srtn);
                
             }
               
         }

         while(true) {
             if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
                 break;
             ID = msgProcess.Process.ID;
             printf("message Recieved id =%d\n",msgProcess.Process.ID);
            
                 Enqueue(Ready_Processes,msgProcess.Process,srtn);
                
                
         }
        
       
   }
}


//========================================================================================================================

//============================================= RR algorithm ============================================================
void RR( struct Queue** Ready_Processes,int quantum)
{
    struct Process *process;
    process->State =0;
    while(true) {
        if(!stillSending && isEmpty(Ready_Processes) ) 
            return;
        if(!isEmpty(Ready_Processes) )
        {
            process=Dequeue(Ready_Processes,rr);
            if( process->State==0 ) {
                if(process->RemainingTime >quantum) {
                    process->FixedTime=process->RunTime;
                    start(process);
                    process->State=1;
                    int past = getClk()+quantum;
                    while(getClk() < past){
                        
                    }
                    process->RemainingTime=process->RemainingTime-quantum;
                    Pause(*process);
                    Enqueue(Ready_Processes,*process,rr);

                }
                else if (process->RemainingTime == 0){
                    finish(*process);
                } else {
                    process->FixedTime=process->RunTime;
                    start(process);
                    int past = getClk()+process->RemainingTime;
                    while(getClk() < past){
                        
                    }
                    finish(*process);
                }
            }
            else {
                if(process->RemainingTime >quantum) {
                    resume(*process);
                    int past = getClk()+quantum;
                    while(getClk() < past){
                        
                    }
                    process->RemainingTime=process->RemainingTime-quantum;
                    Pause(*process);
                    Enqueue(Ready_Processes,*process,rr);
                }
                else if (process->RemainingTime == 0){
                    finish(*process);
                } else {
                    resume(*process);
                    int past = getClk()+process->RemainingTime;
                    while(getClk() < past){
                        
                    }
                    finish(*process);
                }
            }
        }
        while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1){
                break;
            }
            printf("message Recieved id =%d\n",msgProcess.Process.ID);
            if (msgProcess.Process.ID != -1)
                Enqueue(Ready_Processes,msgProcess.Process,rr);
        }
    }
}



void handler(int signum) // THe SIGUSER1 signal handler
{
    stillSending = false;
}









/*        if(!isEmpty(Ready_Processes) )
        {
            process=Dequeue(Ready_Processes,1); //1 stands to HPF it matters in dequeuing 

            ID=process->ID;//get the id of the coming process
            printf("Process with ID : %d has arrived at time : %d and it is going to start now....\n",ID,getClk());
            start(process);//send the process to start which fork a process child 
            printf("the scheduler is going to sleep now at time : %d untill the process is finished...\n",getClk());
            sleep((process->RemainingTime));
            printf("the scheduler has woke up at time : %d and process with ID: %d has finished....\n",getClk() , ID);
            finish(*process);
            if(ID ==-1 && isEmpty(Ready_Processes)) 
                return;
        }
        while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
            {
                break;
            }
            else if(msgProcess.Process.ID != -1)
            {
                
                ID = msgProcess.Process.ID;
                printf("message is Received at time : %d with ID =%d ...\n",getClk(),msgProcess.Process.ID);
                Enqueue(Ready_Processes,msgProcess.Process,hpf);
                break;
            }
            
        }*/

// void SRTN(struct Queue** Ready_Processes) {
//     struct Process *process,*temp;
//     int ID;
//     while(true) {
//         if(!isEmpty(Ready_Processes) )
//         {
//             process=Dequeue(Ready_Processes,srtn);
//              if(ID ==-1 && isEmpty(Ready_Processes)) 
//                 return;
            
//             if (process->State==0) {
//                 start(process);
//                 process->State=1;
//                 sleep(1);
//                 process->RemainingTime=process->RemainingTime -1;

               
//             } else {
//                 resume(*process);
//                 sleep(1);
//                process->RemainingTime=process->RemainingTime -1;
//             }
//             if(process->RemainingTime==0)
//             {
//                 finish(*process);
//             }
//             temp=PeekFront(Ready_Processes);
//             if(!temp )
//             {
//                    if(temp->RemainingTime < process->RemainingTime)
//                    {
//                     pause(*process);
//                     Enqueue(Ready_Processes,msgProcess.Process,srtn);
//                    }
                
//             }
//             else{
//                 Enqueue(Ready_Processes,msgProcess.Process,srtn);
//             }

           
//         }
//         while(true) {
//             if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
//                 break;
//             ID = msgProcess.Process.ID;
//             printf("message Recieved id =%d\n",msgProcess.Process.ID);
//             if (msgProcess.Process.ID != -1)
//                 Enqueue(Ready_Processes,msgProcess.Process,srtn);







                
//         }
        
//     }
// }
/* if(!isEmpty(Ready_Processes) )
        {
            if(checkScheduler==-1)
            {
                process=Dequeue(Ready_Processes,hpf); //get the process with the highest priority in the ready queue
                ID = process->ID;
                if(process->ID==-1)
                return;
                if(process == NULL)
                {
                    return;
                }

                printf("Process with ID : %d has arrived to the scheduler at time : %d and it is going to start now....\n",ID,getClk());

                start(process);
                starttime=getClk();//send the process to start which fork a process child
                checkScheduler = 1;
                printf("The start time is : %d\n",starttime);
                
            }

        }

            if(starttime + (process->RemainingTime) <= getClk())//this is the time the process will finish in
            {
                finish(*process);
                checkScheduler = -1; //the cpu is now free to accept ready process
            }
            else{
                //printf("number of seconds sleeped : %d\n", starttime + (process->RemainingTime) - getClk() - 1);
               sleep(starttime + (process->RemainingTime) - getClk() - 1);
                
            }

        if(msgProcess.Process.ID == -1 && checkScheduler== -1){
            //sleep(50);
            break;
        }*/