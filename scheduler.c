#include "headers.h"

// Global variables
int msgq_id;
int received_processes = 0;
int finished_processes = 0;
int process_count = 0;
int* arrived_arr;
Queue* Ready_Queue;
PCB* running_process = NULL;

void CreateMessageQueue() {
    key_t keyid = ftok("keyfile", 65);
    msgq_id = msgget(keyid, 0666 | IPC_CREAT);

    if (msgq_id == -1) {
        perror("Error in creating message queue");
        exit(-1);
    }
}





PCB* Receive_process() {
    msgbuff message;
    PCB* rec_process = malloc(sizeof(PCB));
    // Try to receive a message
    int rec_val = msgrcv(msgq_id, &message, sizeof(message.process), 0, IPC_NOWAIT);

    if (rec_val != -1) {
        received_processes++;
        printf("Received process: ID=%d, Arrival=%d, Runtime=%d, Priority=%d\n",
               message.process.id,
               message.process.arrivaltime,
               message.process.runningtime,
               message.process.priority);
        rec_process->id = message.process.id;
        rec_process->arrival_time = message.process.arrivaltime;
        rec_process->priority = message.process.priority;
        rec_process->runtime = message.process.runningtime;
        rec_process->waiting_time = 0;
        rec_process->state = READY;
        rec_process->remaining_time = message.process.runningtime;
    } else {
        free(rec_process); // Avoid memory leak
        rec_process = NULL;
    }
    return rec_process;
}

void process_ended(int sig) {
    printf("Process %d has finished\n", running_process->id);
    running_process = NULL;
    finished_processes++;
}

void HPF() {
    while (finished_processes < process_count) {
        // Receive processes
        PCB* current_p = Receive_process();
        while (current_p) {
            enqueue(Ready_Queue, current_p, current_p->priority);
            current_p = Receive_process();
        }

        if (!running_process && !isEmptyQ(Ready_Queue)) {
                running_process = dequeue(Ready_Queue);
                int pid = fork();
                if (pid == 0) {
                    char remaining_time_str[10];
                    sprintf(remaining_time_str, "%d", running_process->remaining_time);
                    execl("./process.out", "process", remaining_time_str, NULL);
                    perror("Error executing process");
                    exit(-1);
                } else if (pid < 0) {
                    perror("Error in fork");
                } else {
                    running_process->pid = pid;
                }
            }
    }
}

void SJF() {
    while (finished_processes < process_count) {
        signal(SIGUSR1, process_ended);

        PCB* current_p = Receive_process();
        while (current_p) {
            enqueue(Ready_Queue, current_p, current_p->runtime);
            current_p = Receive_process();
        }

        if (running_process != NULL) {
            int status;
            pid_t result = waitpid(running_process->pid, &status, WNOHANG);
            if (result == -1) {
                perror("waitpid failed");
            } else if (result == 0) {
                // Process still running
            } else if (result == running_process->pid) {
                printf("Process %d has finished\n", running_process->id);
                running_process = NULL;
                finished_processes++;
            }
        }
    }
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Too few arguments to scheduler\n");
        exit(-1);
    }

    Ready_Queue = createQueue(100);

    // Initialize clock
    initClk();

    int algorithm = atoi(argv[1]);
    process_count = atoi(argv[2]);

    int quantum = 0;
    if (algorithm == 3) quantum = atoi(argv[3]);

    printf("Scheduler started with:\n");
    printf("Algorithm: %d\n", algorithm);
    printf("Process count: %d\n", process_count);
    if (algorithm == 3) printf("Quantum: %d\n", quantum);

    // Create message queue
    CreateMessageQueue();

    // Open log file
    FILE* logfile = fopen("scheduler.log", "w");
    if (!logfile) {
        perror("Error opening log file");
        exit(-1);
    }

    switch (algorithm) {
        case 1:
            HPF();
            break;
        case 2:
            SJF();
            break;
        // case 3:
        //     RR();
        //     break;
    }

    // Cleanup
    fclose(logfile);
    destroyClk(true);

    return 0;
}
