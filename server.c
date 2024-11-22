#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>

struct msgbuff
{
    long mtype;
    char mtext[70];
};
int upqid ,downqid;
void terminate (int signum){
    msgctl(upqid,IPC_RMID,NULL);
    msgctl(downqid,IPC_RMID,NULL);
    exit(0);
}

/* convert upper case to lower case or vise versa */
void conv(char *msg)
{
    int size = strlen(msg);
    for (int i = 0; i < size; i++)
        if (islower(msg[i]))
            msg[i] = toupper(msg[i]);
        else if (isupper(msg[i]))
            msg[i] = tolower(msg[i]);
}

int main()
{
    signal(SIGINT,terminate);
    key_t keyup , keydown;
  
    int rec_val, send_val;
    keyup = ftok("up" , 10);
    upqid=msgget(keyup,IPC_CREAT | 0666);
    keydown = ftok("down" , 10);
    downqid=msgget(keydown,IPC_CREAT | 0666);
    struct msgbuff sent_message;
    struct msgbuff rec_message;

    while(1){
        rec_val = msgrcv(upqid,&rec_message, sizeof(rec_message.mtext),0,!IPC_NOWAIT);
        if(rec_val == -1)
            perror("Error in receive");
        else{
            printf("\nMessage received: %s\n", rec_message.mtext);
            conv(&rec_message.mtext);
            msgsnd(downqid,&rec_message,sizeof(rec_message.mtext),!IPC_NOWAIT);
        }
    }
    return 0;
}
