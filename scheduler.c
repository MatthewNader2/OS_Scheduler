#include "headers.h"
#include <signal.h>

// Global variables
int msgq_id;
int received_processes = 0;
int finished_processes = 0;
int process_count = 0;
int first_arr_proc;
Queue* Ready_Queue;
Queue* Mem_Queue;
PCB* running_process = NULL;
FILE* logfile;
FILE * MemFile;
Node* root ;

// Variables for performance 
float total_runtime = 0;
float total_TA = 0;
float total_WTA = 0;
float total_waiting_time = 0;
float* WTA_values;  



void HPF();
void SJF();
void RR(int quantum);


Node* allocateMemoryWithSplit(Node* root, int size, PCB*b) {
    int roundedSize = roundUpToPowerOfTwo(size);  // Round the size up to the nearest power of 2
    printf("Requesting %d bytes, rounded to %d bytes.\n", size, roundedSize);

    // First, try to find an exact match for the requested size
    Node* block = findFreeBlock(root, roundedSize);

    // If we found a block of the exact size, allocate it
    if (block != NULL && block->size == roundedSize) {
        // Allocate the block by assigning a PCB
        block->pcb =b ;

        // printf("The process %d has allocated memory %d\n",b->id,block->size);

        fprintf(MemFile, "At time %d allocated %d bytes for process %d at block of size %d start from %d to %d\n",
                getClk(), b->size , b->id,block->size,block->begin,block->begin+block->size-1);

        ar_size=remove_element(ar,ar_size,block->size);

        return block;
    }
    
    // If no exact match, find a larger block and split it
    if (block == NULL || block->size > roundedSize) {
       

        
            // Split the larger block into smaller blocks
            splitBlock(block);
            // Allocate memory from the smaller blocks after splitting
            return allocateMemoryWithSplit(root, size, b);
            }
            else {
            // No suitable block available
            printf("Memory allocation failed: No suitable block available.\n");
            return NULL;
                 }
    

    return NULL;  // Fallback if no suitable block was found
}


void deallocate(Node* root, PCB* pcb) {
    
    if (root == NULL) return;
    
    // If the block is occupied by the given PCB, deallocate it
    if (root->pcb == pcb) {
        printf("the process %d has deallocated %d\n",pcb->id,root->size);
        fprintf(MemFile, "At time %d freed %d bytes for process %d at block of size %d from %d to %d\n ",
            getClk(), pcb->size , pcb->id,root->size,root->begin,root->size + root->begin-1);

        ar_size = add(ar,ar_size,root->size,max_size);
        root->pcb = NULL;
        return;
    }

    // Recurse into left and right children
    deallocate(root->left, pcb);
    deallocate(root->right, pcb);

    // Merge buddies if both are free
    if (root->left && root->right &&
        root->left->pcb == NULL && root->right->pcb == NULL &&
        root->left->left == NULL && root->right->right == NULL) {
        fprintf(MemFile, "At time %d two blocks of size %d has been merged in one block of size %d\n",
            getClk(), root->size/2,root->size);
        ar_size=remove_element(ar,ar_size,root->left->size);
        ar_size=remove_element(ar,ar_size,root->left->size);
        ar_size=add(ar,ar_size,root->size,max_size);
        free(root->left);
        free(root->right);
        root->left = NULL;
        root->right = NULL;
    }
}





void CreateMessageQueue() {
    key_t keyid = ftok("keyfile", 65);
    msgq_id = msgget(keyid, 0666 | IPC_CREAT);

    if (msgq_id == -1) {
        perror("Error in creating message queue");
        exit(-1);
    }
}

void Log_Process_Event(PCB* process, const char* state) {
    int time = getClk();
    int arr = process->arrival_time;
    int total = process->runtime;
    int remain = process->remaining_time;
    int wait = time - arr - (process->runtime - process->remaining_time);

    if (strcmp(state, "finished") == 0) {
        int TA = time - arr;
        float WTA = (float)TA / process->runtime;
        WTA = ((int)(WTA * 100 + 0.5)) / 100.0; // Round to 2 decimal places

        fprintf(logfile, "At time %d process %d %s arr %d total %d remain %d wait %d TA %d WTA %.2f\n",
                time, process->id , state, arr, total, remain, wait, TA, WTA);
    } else {
        fprintf(logfile, "At time %d process %d %s arr %d total %d remain %d wait %d\n",
                time, process->id, state, arr, total, remain, wait);
    }

    fflush(logfile); 
}

void Check_Process_Termination() {
    termination_msgbuff message;
    int rec_val = msgrcv(msgq_id, &message, sizeof(message) - sizeof(long), 5, IPC_NOWAIT);

    if (rec_val != -1) {
       
        if (running_process && running_process->pid == message.pid) {
            
            running_process->state = FINISHED;
            running_process->remaining_time=0;
           
            Log_Process_Event(running_process, "finished");

           
            int TA = getClk() - running_process->arrival_time;
            float WTA = (float)TA / running_process->runtime;
            total_TA += TA;
            total_WTA += WTA;
            total_waiting_time += (running_process->start_time - running_process->arrival_time);

            
            WTA_values[finished_processes] = WTA;
            deallocate(root,running_process);
            free(running_process); 
            running_process = NULL;
            finished_processes++;
        } else {
            printf("Unknown process with PID %d reported termination.\n", message.pid);
        }
    }
}


PCB* Receive_process() {
    process_msgbuff message;
    
    if(!isEmptyQ(Mem_Queue)){
        Queue * temp = createQueue(50);
        PCB * dec;
        PCB* te ;
        bool T = true;
        
        while (Mem_Queue->front)
        {
        
        te = dequeue(Mem_Queue);
        
        if(search(ar,ar_size,te->size)!=-1&& T){
            dec =te;
            allocateMemoryWithSplit(root,te->size,te);
            T =false;
            
            continue;
        }
        else
        {
            enqueue(temp,te);
        }

        }
        while (temp->front)
        {
            te =dequeue(temp);
            enqueue(Mem_Queue,te);
        }
        free(temp);
        if (T==false){
            printf("\n");
        return dec;
        }
    }
    PCB* rec_process = malloc(sizeof(PCB));
 
    int rec_val = msgrcv(msgq_id, &message, sizeof(message.process), 1, IPC_NOWAIT);

    if (rec_val != -1) {
        if (received_processes==0){
            first_arr_proc=message.process.arrivaltime;
        }
        
        received_processes++;
     
        rec_process->id = message.process.id;
        rec_process->arrival_time = message.process.arrivaltime;
        rec_process->priority = message.process.priority;
        rec_process->runtime = message.process.runningtime;
        rec_process->waiting_time = 0;
        rec_process->state = READY;
        rec_process->remaining_time = message.process.runningtime;
        rec_process->start_time = -1;
        rec_process->last_run = -1;
        rec_process->size=message.process.size;
        total_runtime += message.process.runningtime; 
       
       if(search(ar,ar_size,rec_process->size)==-1)
        {
            enqueue(Mem_Queue,rec_process);
            return NULL;
        }
        allocateMemoryWithSplit(root,rec_process->size,rec_process);
        
    } else {
        free(rec_process); 
        rec_process = NULL;
    }
    return rec_process;
}


float Round(float var) {
    return ((int)(var * 100 + 0.5)) / 100.0;
}

void ComputePerformanceMetrics() {
    FILE* perf_file = fopen("scheduler.perf", "w");
    if (!perf_file) {
        perror("Error opening performance file");
        exit(-1);
    }

    int total_time = getClk() - first_arr_proc; 
    float cpu_utilization = ((float)total_runtime / total_time) * 100;
    cpu_utilization = Round(cpu_utilization);

    float avg_WTA = total_WTA / process_count;
    avg_WTA = Round(avg_WTA);

    float avg_waiting = (float)total_waiting_time / process_count;
    avg_waiting = Round(avg_waiting);

   
    float sum_squared_diff = 0;
    for (int i = 0; i < process_count; i++) {
        float diff = WTA_values[i] - avg_WTA;
        sum_squared_diff += diff * diff;
    }
    float std_WTA = sqrt(sum_squared_diff / process_count);
    std_WTA = Round(std_WTA);

    fprintf(perf_file, "CPU utilization = %.2f%%\n", cpu_utilization);
    fprintf(perf_file, "Avg WTA = %.2f\n", avg_WTA);
    fprintf(perf_file, "Avg Waiting = %.2f\n", avg_waiting);
    fprintf(perf_file, "Std WTA = %.2f\n", std_WTA);

    fclose(perf_file);
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Too few arguments to scheduler\n");
        exit(-1);
    }

    root = createNode(1024,NULL,0);



    Ready_Queue = createQueue(100);
    
    Mem_Queue = createQueue(50);

    initClk();

    int algorithm = atoi(argv[1]);
    process_count = atoi(argv[2]);

    
    WTA_values = (float*)malloc(sizeof(float) * process_count);
    if (WTA_values == NULL) {
        perror("Error allocating memory for WTA_values");
        exit(-1);
    }

    int quantum = 0;
    if (algorithm == 3) quantum = atoi(argv[3]);

    printf("Scheduler started with:\n");
    printf("Algorithm: %d\n", algorithm);
    printf("Process count: %d\n", process_count);
    if (algorithm == 3) printf("Quantum: %d\n", quantum);

    // Create message queue
    CreateMessageQueue();

    // Open log file
    MemFile =fopen("Memeory.log","w");
    logfile = fopen("scheduler.log", "w");
    if (!logfile) {
        perror("Error opening log file");
        exit(-1);
    }
    fprintf(logfile, "#At time x process y state arr w total z remain y wait k\n");

    switch (algorithm) {
        case 1:
            HPF();
            break;
        case 2:
            SJF();
            break;
        case 3:
            RR(quantum);
            break;
        default:
            printf("Invalid algorithm selected.\n");
            exit(-1);
    }

  

  
    ComputePerformanceMetrics();
    printf("The Scheduling is ended..\n");
    // Cleaning
    fclose(logfile);
    fclose(MemFile);
    free(WTA_values);
    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    destroyClk(true);

    return 0;
}



void HPF() {
    while (finished_processes < process_count) {
         if (running_process == NULL && !isEmptyQ(Ready_Queue)) {
            running_process = dequeue(Ready_Queue);
            if (running_process->state == READY) {
                // Start the process
                int pid = fork();
                if (pid == 0) {
                    // Child process
                    char remaining_time_str[10];
                    sprintf(remaining_time_str, "%d", running_process->remaining_time);

                    // printf("getclk :  %d\n", getClk());
                    execl("./process", "process", remaining_time_str, NULL);
                    perror("Error executing process");
                    exit(-1);
                } else if (pid < 0) {
                    perror("Error in fork");
                } else {
                    // Parent process
                    running_process->pid = pid;
                    running_process->state = RUNNING;
                    running_process->start_time = getClk();
                    running_process->last_run = getClk();
                    // printf("Process %d started with PID %d at time %d.\n", running_process->id, pid, getClk());

                    // Log the process start
                    Log_Process_Event(running_process, "started");
                }
            } else if (running_process->state == BLOCKED) {
                // Resume the process
                kill(running_process->pid, SIGCONT);
                running_process->state = RUNNING;
                running_process->last_run = getClk();
                

                // Log the process resume
                Log_Process_Event(running_process, "resumed");
            }
        }
        
        PCB* current_p = Receive_process();
        while (current_p) {
            enqueue(Ready_Queue, current_p); // Enqueue based on priority
            current_p = Receive_process();
        }

        // Check for any processes that have terminated
        Check_Process_Termination();

        // Check for preemption
        if (running_process != NULL && !isEmptyQ(Ready_Queue)) {
            PCB* highest_priority_process = front(Ready_Queue);
            if (highest_priority_process->priority < running_process->priority) {
            
                // Send SIGSTOP to running process
                kill(running_process->pid, SIGSTOP);

                // Update running process's state and remaining time
                int current_time = getClk();
                int elapsed_time = current_time - running_process->last_run;
                running_process->remaining_time -= elapsed_time;
                running_process->state = BLOCKED;

             
                Log_Process_Event(running_process, "stopped");

                // Enqueue running process back to Ready Queue
                enqueue(Ready_Queue, running_process);

               
                running_process = dequeue(Ready_Queue);
                if (running_process->state == READY) {
                    // Start the process
                    int pid = fork();
                    if (pid == 0) {
                        // Child process
                        char remaining_time_str[10];
                        sprintf(remaining_time_str, "%d", running_process->remaining_time);
                        execl("./process", "process", remaining_time_str, NULL);
                        perror("Error executing process");
                        exit(-1);
                    } else if (pid < 0) {
                        perror("Error in fork");
                    } else {
                        // Parent process
                        running_process->pid = pid;
                        running_process->state = RUNNING;
                        running_process->start_time = getClk();
                        running_process->last_run = getClk();
                    

                        // Log the process start
                        Log_Process_Event(running_process, "started");
                    }
                } else if (running_process->state == BLOCKED) {
                    // Resume the process
                    kill(running_process->pid, SIGCONT);
                    running_process->state = RUNNING;
                    running_process->last_run = getClk();
               

                    // Log the process resume
                    Log_Process_Event(running_process, "resumed");
                }
            }
        }

      
  
        
    }
}

void SJF() {
    while (finished_processes < process_count) {
       
        PCB* current_p = Receive_process();
        while (current_p) {
            enqueue_SJF(Ready_Queue, current_p); 
            current_p = Receive_process();
        }

        // Check for any ended processes
        Check_Process_Termination();

        // Start a new process 
        if (running_process == NULL && !isEmptyQ(Ready_Queue)) {
            running_process = dequeue(Ready_Queue);
            int pid = fork();
            if (pid == 0) {
                // Child process
                char remaining_time_str[10];
                sprintf(remaining_time_str, "%d", running_process->remaining_time);
                execl("./process", "process", remaining_time_str, NULL);
                perror("Error executing process");
                exit(-1);
            } else if (pid < 0) {
                perror("Error in fork");
            } else {
              
                running_process->pid = pid;
                running_process->state = RUNNING;
                running_process->start_time = getClk();
                running_process->last_run = getClk();
                

              
                Log_Process_Event(running_process, "started");
                
            }
        }
       

      
        
    }
}

void RR(int quantum) {
    while (finished_processes < process_count) {
        
        PCB* current_p = Receive_process();
        while (current_p) {
            enqueue_RR(Ready_Queue, current_p); // Enqueue in FCFS order
            current_p = Receive_process();
        }

        
        Check_Process_Termination();

        int current_time = getClk();

        // Check if time quantum has expired for the running process
        if (running_process != NULL) {
            int elapsed_time = current_time - running_process->last_run;
            if (elapsed_time >= quantum) {
                
                kill(running_process->pid, SIGSTOP);
                // Update remaining time
                running_process->remaining_time -= elapsed_time;
                if (running_process->remaining_time > 0) {
                     running_process->state = BLOCKED;
                    // Re-enqueue the process
                    enqueue_RR(Ready_Queue, running_process);
                }

                    // Log the process stop
                    Log_Process_Event(running_process, "stopped");
              
                running_process = NULL;
            }
        }

        // Start a new process if none is running
        if (running_process == NULL && !isEmptyQ(Ready_Queue)) {
            running_process = dequeue(Ready_Queue);
            if (running_process->state == READY) {
                // Start the process
                int pid = fork();
                if (pid == 0) {
                    // Child process
                    char remaining_time_str[10];
                    sprintf(remaining_time_str, "%d", running_process->remaining_time);
                    execl("./process", "process", remaining_time_str, NULL);
                    perror("Error executing process");
                    exit(-1);
                } else if (pid < 0) {
                    perror("Error in fork");
                } else {
                    // Parent process
                    running_process->pid = pid;
                    running_process->state = RUNNING;
                    running_process->start_time = getClk();
                    running_process->last_run = getClk();
                   
                    Log_Process_Event(running_process, "started");
                }
            } else if (running_process->state == BLOCKED) {
                // Resume the process
                kill(running_process->pid, SIGCONT);
                running_process->state = RUNNING;
                running_process->last_run = getClk();
                

                // Log the process resume
                Log_Process_Event(running_process, "resumed");
            }
        }
        sleep(1);
     

    }
}

