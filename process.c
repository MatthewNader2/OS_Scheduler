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
    initClk();

    // Check if the remaining time is passed as an argument
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s remaining_time\n", argv[0]);
        exit(-1);
    }

    int remaining_time = atoi(argv[1]);
    int last_time = getClk();

    printf("Process %d started at time %d with remaining time %d\n", getpid(), last_time, remaining_time);

    while (remaining_time > 0)
    {
        int current_time = getClk();
        if (current_time > last_time)
        {
            remaining_time -= current_time - last_time;
            last_time = current_time;
            printf("Process %d at time %d, remaining time %d\n", getpid(), current_time, remaining_time);
        }
        sleep(1);
    }

    // Process has finished execution
    printf("Process %d has finished execution at time %d.\n", getpid(), getClk());
    destroyClk(false);
    exit(0);
}
