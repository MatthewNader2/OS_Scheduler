#include "headers.h"
// Global variables 
int num_processes,finished_processes=0;
Queue* Ready_Queue;
PCB* runningProcess;

void startProcess(PCB* p)
{
    p->start_time = getClk();
    p->waiting_time += p->start_time - p->arrival_time;
    

}
struct PCB* createProcess()
{

    message* msg;
    int rec_val = msgrcv(msgq_id, &msg, sizeof(message), 0, IPC_NOWAIT);
    if (rec_val == -1)
    {
        printf("Error in receive\n");
        return NULL;
    }
    else
    {
        PCB *newProcess = malloc(sizeof(struct PCB));
        newProcess->id = msg->id;
        newProcess->arrival_time = msg->arr_time;
        newProcess->runtime = msg->runningtime;
       
        
       // printf("\nMessage received: at time %d\n",getClk());
        return newProcess;
    }
}
void HPF(){
    printf("Running HPF algorithm");
    while(finished_processes < num_processes){
        while(arrived_processes[getClk()]){
            PCB* current_p = CreateProcess();
            while(current_p){
            arrived_process[getClk()--]
            push(&Ready_Queue , current_p , current_p->priority)
            current_p = CreateProcess();
            }
        }
        if(!runningProcess && !isEmpty(&readyQueue))
        {
            runningProcess = peek(&Ready_Queue);
            pop(&Ready_Queue);
            // *remainingTime = runningProcess->brust;
            startProcess(runningProcess);
            int pid = fork();
            if (pid == -1)
            {
  	            scanf("error in fork in scheduler\n");
                kill(getpgrp(), SIGKILL);
            }
            if(pid == 0)
            {
                printf("%d\n", runningProcess->id);
                execl("./process","process", NULL);
            }
        }
        //int currClk = getClk();
        // if(*remainingTime == 0 && runningProcess)
    }
};

void SJF(){};
void RR(){};
int main(int argc, char * argv[])
{
    int arrived_processes={2,3,4};
    int rec_val;
    int algorthim =1;
    // algorthim = atoi(argv[1]);
  

    
    switch(algorthim){
        case 1:
            HPF();
            break;
        case 2:
            SJF();
            break;
        case 3:
            RR();
            break;


    }


    
    // initClk();
  
    
   
    //upon termination release the clock resources
    
    // destroyClk(true);
     
}
