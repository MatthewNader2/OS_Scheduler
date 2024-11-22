#include "headers.h"

void clearResources(int);



int msq_id;

int main(int argc, char* argv[]) {
    signal(SIGINT, clearResources);

    // Initialize message queue
    key_t keyid = ftok("keyfile", 55);
    msq_id = msgget(keyid, 0666 | IPC_CREAT);
    if (msq_id == -1) {
        perror("Error in creating the message queue");
        exit(-1);
    }

    FILE* file = fopen("processes.txt", "r");
    if (!file) {
        perror("Error opening processes file");
        exit(-1);
    }

    int Proc_num = 0;
    char buffer[200];
    fgets(buffer, sizeof(buffer), file); // Skip the first line

    while (fgets(buffer, sizeof(buffer), file)) {
        Proc_num++;
    }
    fclose(file);

    file = fopen("processes.txt", "r");
    processData* procs = malloc(sizeof(processData) * Proc_num);
    if (!procs) {
        perror("Failed to allocate memory");
        exit(-1);
    }

    fgets(buffer, sizeof(buffer), file); // Skip the first line again
    int count = 0;

    while (fscanf(file, "%d\t%d\t%d\t%d", &procs[count].id, &procs[count].arrivaltime, 
                  &procs[count].runningtime, &procs[count].priority) == 4) {
        count++;
    }
    fclose(file);

    // Ask the user for the chosen scheduling algorithm
    int algorithm;
    printf("Choose a scheduling algorithm:\n");
    printf("1. Non-preemptive Highest Priority First (HPF).\n");
    printf("2. Shortest Remaining Time Next (SRTN).\n");
    printf("3. Round Robin (RR).\n");
    scanf("%d", &algorithm);

    int timeSlot = 0;
    if (algorithm == 3) {
        printf("Enter the time slot for Round Robin: ");
        scanf("%d", &timeSlot);
    }
     int pid_c = fork();
    if (pid_c == -1) {
        perror("Error in creating clock process");
    } else if (pid_c == 0) {
        execl("./clk.out", "clk", NULL);
    }

    initClk();

    // Create scheduler and clock processes
    int pid_s = fork();
    if (pid_s == -1) {
        perror("Error in creating scheduler process");
    } else {
        char proc_str[10], algo_str[10], time_str[10];
        snprintf(proc_str, sizeof(proc_str), "%d", Proc_num);
        snprintf(algo_str, sizeof(algo_str), "%d", algorithm);

        if (algorithm == 3) {
            snprintf(time_str, sizeof(time_str), "%d", timeSlot);
            execl("./scheduler.out", "scheduler", proc_str, algo_str, time_str, NULL);
        } else {
            execl("./scheduler.out", "scheduler", proc_str, algo_str, NULL);
        }
    }

   

    // Send processes to the scheduler at the appropriate time
    int snd_num = 0;
    while (snd_num < Proc_num) {
        if (procs[snd_num].arrivaltime == getClk()) {
            msgbuff msg;
            msg.mtype = 1;
            msg.process = procs[snd_num];
            msgsnd(msq_id, &msg, sizeof(msg.process), !IPC_NOWAIT);
            snd_num++;
        }
    }

    waitpid(pid_s, NULL, 0);
    destroyClk(true);
    free(procs);

    return 0;
}

void clearResources(int signum) {
    msgctl(msq_id, IPC_RMID, NULL);
    destroyClk(true);
    exit(0);
}