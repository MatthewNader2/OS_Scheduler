#include "headers.h"

// Global variables
int msgq_id;
int received_processes = 0;
int finished_processes = 0;
int process_count = 0;
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
    process_msgbuff message;
    PCB* rec_process = malloc(sizeof(PCB));
    // Try to receive a message of type PROCESS_ARRIVAL_MSG (process arrival)
    int rec_val = msgrcv(msgq_id, &message, sizeof(message.process), PROCESS_ARRIVAL_MSG, IPC_NOWAIT);

    if (rec_val != -1) {
        received_processes++;
        printf("Received process: ID=%d, Arrival=%d, Runtime=%d, Priority=%d\n",
               message.process.id,
               message.process.arrivaltime,
               message.process.runningtime,
               message.process.priority);
        rec_process->pid = message.process.id;
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

// Function to receive termination messages from processes
void Check_Process_Termination() {
    termination_msgbuff message;
    int rec_val = msgrcv(msgq_id, &message, sizeof(message) - sizeof(long), PROCESS_TERMINATION_MSG, IPC_NOWAIT);

    if (rec_val != -1) {
        // Process has notified termination
        printf("Scheduler received termination message from process with PID %d.\n", message.pid);

        if (running_process && running_process->pid == message.pid) {
            printf("Process %d has finished at time %d.\n", running_process->pid, getClk());
            running_process->state = FINISHED;
            free(running_process); // Free the PCB memory
            running_process = NULL;
            finished_processes++;
        } else {
            printf("Unknown process with PID %d reported termination.\n", message.pid);
        }
    }
}

void SJF() {
    // Since processes notify termination, no need for SIGCHLD handler
    while (finished_processes < process_count) {
        // Receive any new processes
        PCB* current_p = Receive_process();
        while (current_p) {
            enqueue(Ready_Queue, current_p);
            current_p = Receive_process();
        }

        // Check for any processes that have terminated
        Check_Process_Termination();

        // Start a new process if none is running
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
                // Parent process
                running_process->pid = pid;
                running_process->state = RUNNING;
                running_process->start_time = getClk();
                printf("Process %d started with PID %d at time %d.\n", running_process->pid, pid, getClk());
            }
        }

        // Sleep to prevent busy waiting
        sleep(1);
    }
}

void RR(int quantum) {
    while (finished_processes < process_count) {
        // Receive any new processes
        PCB* current_p = Receive_process();
        while (current_p) {
            // For RR, processes are enqueued in FCFS order
            enqueue(Ready_Queue, current_p);
            current_p = Receive_process();
        }

        // Check for any processes that have terminated
        Check_Process_Termination();

        int current_time = getClk();

        // Check if time quantum has expired for the running process
        if (running_process != NULL) {
            int elapsed_time = current_time - running_process->last_run;
            if (elapsed_time >= quantum) {
                // Time quantum expired
                // Stop the current running process
                kill(running_process->pid, SIGSTOP);
                // Update remaining time
                running_process->remaining_time -= elapsed_time;
                if (running_process->remaining_time <= 0) {
                    // Process has finished, but it will notify us
                    printf("Process %d has finished execution.\n", running_process->pid);
                } else {
                    // Update process state
                    running_process->state = BLOCKED;
                    // Re-enqueue the process
                    enqueue(Ready_Queue, running_process);
                }
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
                    running_process->last_run = getClk();
                    printf("Process %d started with PID %d at time %d.\n", running_process->pid, pid, getClk());
                }
            } else if (running_process->state == BLOCKED) {
                // Resume the process
                kill(running_process->pid, SIGCONT);
                running_process->state = RUNNING;
                running_process->last_run = getClk();
                printf("Process %d resumed at time %d.\n", running_process->pid, getClk());
            }
        }

        sleep(1);
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
            // Implement HPF if needed
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

    // Wait for any remaining child processes
    while (wait(NULL) > 0);

    // Cleanup
    fclose(logfile);
    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    destroyClk(true);

    return 0;
}
