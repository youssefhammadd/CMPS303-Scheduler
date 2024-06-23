#include "headers.h"

 int NumberofProcesses;
 int SendingQueue;
void clearResources(int);
int numberofProcesses(char* file_name);//this function is to get the number of processes inside the text file



int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
//============================= creating ProcesssTable Matrix =========================================================
    //getting the number of processes inside the processes table

      int rows=numberofProcesses("processes.txt");
      NumberofProcesses=rows;

    //number of columns of the process table is 4 which are the id and arrival and run time and the priority
      int columns=4;

    //creating the process table matrix that will be filled from the text file
    int ProcessTable[rows][columns];
//======================================================================================================================

//============================ reading the processes from the text file ================================================
    // 1. Read the input files.
    FILE *file_ptr;
    char line[500]; // Assuming each line has at most 128 characters



    // Open the file in read only mode
    file_ptr = fopen("processes.txt", "r");

    // Check if file opened successfully
    if (file_ptr == NULL) {
        printf("Unable to open the file....\n");
        return 1;
    }
    //ignore the comment line
    fgets(line,sizeof(line),file_ptr);

    int RowNumber=0;

    // Read and print each line until end of file is reached
    while (fgets(line, sizeof(line), file_ptr) != NULL) {
        int id, arrival, runtime, priority;
        sscanf(line, "%d %d %d %d", &id, &arrival, &runtime, &priority); // parse each number alone form the read line
        ProcessTable[RowNumber][0] = id;
        ProcessTable[RowNumber][1] = arrival;
        ProcessTable[RowNumber][2] = runtime;
        ProcessTable[RowNumber][3] = priority;

        //after reading a row from the file read from the next row
        RowNumber++;
    }

//    for(int i=0;i<rows;i++)
//     {
//         printf("%d  %d  %d  %d\n",ProcessTable[i][0],ProcessTable[i][1],ProcessTable[i][2],ProcessTable[i][3]);
//     }

    // Close the file
    fclose(file_ptr);


//======================================================================================================================



//========================================================================================================================
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    int algorithmNum;
    int quantumNum=-1;
    printf("\nPlease enter algorithm number(1 for HPF , 2 for SRTN , 3 for RR): \n");
    scanf("%d", &algorithmNum);
    if (algorithmNum == 3)
    {
        printf("\nPlease enter quantum number: \n");
        scanf("%d", &quantumNum);
    }
    if (algorithmNum < 1 || algorithmNum > 3)
    {
        printf("\nWrong algorithm number!\n");
        exit(-1);
    }
//========================================================================================================================
    // 3. Initiate and create the scheduler and clock processes.(both are childs from Process_generator)

    char clkBuffer[500];
    getcwd(clkBuffer, sizeof(clkBuffer));//this function gets the current working directory of the calling process and store it inside the clkBuffer
    char* CLK_PATH = strcat(clkBuffer, "/clk.out"); //this function to concatenate the two strings to each other to have the path of the CLK.out to execute it

    //repeat the same for the Scheduler to get it's path
    char schedulerBuffer[500];
    getcwd(schedulerBuffer, sizeof(schedulerBuffer));
    char* SCHEDULER_PATH = strcat(schedulerBuffer, "/scheduler.out");


    int CLKid = fork();
    int SCHid;
    if (CLKid == 0){
        execl(CLK_PATH, "clk.out", NULL);//to replace the current process image with a new program to execute the clk.out
    }
    else
    {
        
        SCHid = fork();
        char algorithmNumChar[sizeof(int)];
        sprintf(algorithmNumChar, "%d", algorithmNum);//to convert the choosen algorithm number into string

        char quantumNumChar[sizeof(int)];
        sprintf(quantumNumChar, "%d", quantumNum);

        char Process_generator_ID[sizeof(getpid())];
        sprintf(Process_generator_ID, "%d", getpid());

        char NumberofProcessesChar[sizeof(int)];
        sprintf(NumberofProcessesChar, "%d", NumberofProcesses);
        if (SCHid == 0) {
            return execl(SCHEDULER_PATH, "scheduler.out", &algorithmNumChar,&quantumNumChar,&Process_generator_ID,&NumberofProcessesChar, NULL);
        }
    }
//========================================================================================================================
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop

for(int i=0;i<rows;i++)
 {
         printf("%d  %d  %d  %d\n",ProcessTable[i][0],ProcessTable[i][1],ProcessTable[i][2],ProcessTable[i][3]);
           }

    //code stops here, if you prinf after current time its is not displayed




    // 5. Create a data structure for processes and provide it with its parameters.

    struct Process Processes[NumberofProcesses];//array of processes to
    for(int i=0 ; i<NumberofProcesses ;i++)
    {
       Processes[i].ID = ProcessTable[i][0];

       Processes[i].ArrivalTime = ProcessTable[i][1];

       Processes[i].RunTime = ProcessTable[i][2];

       Processes[i].RemainingTime = ProcessTable[i][2];

       Processes[i].Priority = ProcessTable[i][3];

       Processes[i].State = 0;
    }

     // 6. Send the information to the scheduler at the appropriate time.(need to share memory or to communicate with the scheduler generally to send the process at the proper time)
        //i will send all the process to the scheduler and the scheduler suppose to store them in the ready queue using shared queue

        key_t key=ftok("keyfile",msgQueue);
        SendingQueue=msgget(key,0666| IPC_CREAT);//created the queue that will send the process through

        struct msgbuff msgProcess;//the process will be sended through this message

        //now i'm going to send the processes one by one when there arrival time come
        int counter =0 ;

        while(counter != NumberofProcesses)
        {
            int CurrentTime=getClk();//current time
            //know the next arrival time for a process
            int ComingProcessTime = Processes[counter].ArrivalTime;
            int WaitingNextProcess = ComingProcessTime - CurrentTime;//the waiting time till the next process arrives
            if(algorithmNum == 1)
                sleep(WaitingNextProcess-0.4);
            else
                sleep(WaitingNextProcess);

            msgProcess.Process = Processes[counter];
            msgProcess.mtype=1;
            int send_val = msgsnd(SendingQueue, &msgProcess, sizeof(msgProcess.Process), !IPC_NOWAIT);
            printf("the message has been send\n");


            counter++;
        }
        //after sending all the processes i need to let the scheduler know so i will send in the message queue a process with id=-1
        struct Process TempProcess;
        //TempProcess.ID=-1;
       // msgProcess.Process=TempProcess;
        //int send_val = msgsnd(SendingQueue, &msgProcess, sizeof(msgProcess), !IPC_NOWAIT);
         kill(SCHid, SIGUSR1);
         sleep(50);



    // 7. Clear clock resources
    //wait for the scheduler to finish working then terminate the process generator
    int stat_loc;
    int sid = waitpid(SCHid,&stat_loc,0);// wait until the scheduler exits or is destroyed
    //when the scheduler is finished , interrupt the clk now
    kill(CLKid, SIGINT);
    destroyClk(true);
}

 int numberofProcesses(char* file_name)
{
    FILE *file_ptr;
    int count = 0;
    char line[500];

    // Open the file in read mode
    file_ptr = fopen("processes.txt", "r");

    // Check if file opened successfully
    if (file_ptr == NULL) {
        printf("Unable to open the file...\n");
        return -1; // Return -1 to indicate error
    }
    //ignore the first line(comment) by reading it without counting it as a line
    fgets(line,sizeof(line),file_ptr);


    // Count the number of newline characters
    while (fgets(line,sizeof(line),file_ptr)) {

            count++;

    }

    // Close the file
    fclose(file_ptr);

    return count;
}





void clearResources(int signum)
{
    printf("process generator is clearing resources now...\n");
    msgctl(SendingQueue, IPC_RMID, NULL);
    destroyClk(true);
    kill(getpid(), SIGKILL);

}
