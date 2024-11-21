#include "headers.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

int remaining_time;
int msgq_id;

struct msgbuff
{
    long mtype;
    int pid;
    int remaining_time;
};

void terminateProcess(int signum)
{
    destroyClk(false);
    raise(SIGKILL);
}

int main(int argc, char *argv[])
{
    // Check if the remaining time is passed as an argument
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s remaining_time\n", argv[0]);
        exit(-1);
    }

    remaining_time = atoi(argv[1]);

    // Get message queue
    key_t msgq_key = ftok("keyfile", 65);
    msgq_id = msgget(msgq_key, 0666 | IPC_CREAT);
    if (msgq_id == -1)
    {
        perror("Error in creating message queue");
        exit(-1);
    }

    // Install signal handler for termination
    signal(SIGUSR1, terminateProcess);

    clock_t start_time = getClk();
    clock_t elapsed_time;

    while (1)
    {
        elapsed_time = getClk() - start_time;
        double cpu_time_used = ((double)elapsed_time) / CLOCKS_PER_SEC;
        int time_left = remaining_time - (int)cpu_time_used;

        if (time_left <= 0)
        {
            // Process has finished execution

            // Send message to scheduler
            struct msgbuff message;
            message.mtype = 1; // You can use a specific type if needed
            message.pid = getpid();
            message.remaining_time = 0; // Process has finished

            if (msgsnd(msgq_id, &message, sizeof(message) - sizeof(long), !IPC_NOWAIT) == -1)
            {
                perror("Error in sending message");
            }

            printf("Process %d has finished execution.\n", getpid());
            raise(SIGUSR1);
        }
        else
        {
            // Optionally, send update to scheduler about remaining time
            struct msgbuff message;
            message.mtype = 1;
            message.pid = getpid();
            message.remaining_time = time_left;

            if (msgsnd(msgq_id, &message, sizeof(message) - sizeof(long), !IPC_NOWAIT) == -1)
            {
                perror("Error in sending message");
            }
        }
    }
}
