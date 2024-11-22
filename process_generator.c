#include "headers.h"

void clearResources(int);

struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};

int msq_id;

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    key_t keyid;
    keyid=ftok("k_file",55);
    msq_id = msgget(keyid,0666 | IPC_CREAT);
    if (msq_id == -1)
    {
        perror("Error in creating the message queue\n");
        exit(-1);
    }
    else
        printf("Message Queue created successfully!\n");

    FILE * file =fopen("processes.txt","r");
    
    int Proc_num =0;
    char buffferr[200];
    fgets(buffferr,sizeof(buffferr),file);

    while(fgets(buffferr,sizeof(buffferr),file)!= NULL){
        

        Proc_num++;
        }
    fclose(file);
    file = fopen("processes.txt","r");

    struct processData *procs = malloc(sizeof(struct processData) * Proc_num);
if (procs == NULL) {
    perror("Failed to allocate memory");
    exit(1);
}

fgets(buffferr,sizeof(buffferr),file);// remove the first line

int count =0;

while(fscanf(file, "%d\t%d\t%d\t%d",&procs[count].id , &procs[count].arrivaltime, &procs[count].runningtime, &procs[count].priority)==4)
{

count++;

}
fclose(file);



    
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
 int algorithm;
    printf("-------------------------------\n");
    printf("Choose a scheduling algorithm:\n");
    printf("1. Non-preemptive Highest Priority First (HPF).\n");
    printf("2. Shortest Remaining time Next (SRTN).\n");
    printf("3. Round Robin (RR).\n");
    scanf("%d", &algorithm);
    printf("-------------------------------\n");

    int timeSlot;
    if (algorithm == 3) {
        printf("Enter the time slot for Round Robin: ");
        scanf("%d", &timeSlot);
    }



    // 3. Initiate and create the scheduler and clock processes.
int pid_s = fork();
if (pid_s == -1) 
        perror("Error in creating scheduler process\n");
    else if (pid_s == 0) {
        system("gcc -o scheduler.out scheduler.c ");
       
        char n_str[10], a_str[10], t_str[10];

        snprintf( n_str, sizeof(n_str),"%d",Proc_num);
         snprintf( a_str, sizeof(a_str),"%d",algorithm);
          
        if (algorithm == 3){
            snprintf( t_str, sizeof(t_str),"%d",timeSlot);
            execl("./scheduler.out", n_str,a_str,t_str, NULL);}
        else
            execl("./scheduler.out", n_str, a_str, NULL);
    }
int pid_c=fork();
if (pid_c == -1) 
        perror("Error in creating clock process\n");
    else if (pid_c == 0) {
        system("gcc  -o clk.out clk.c ");
        
        execl("./clk.out", "clk", NULL);
       }

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.

    // 6. Send the information to the scheduler at the appropriate time.
int snd_num=0;

    while(snd_num<Proc_num)
    {
        if(procs[snd_num].arrivaltime==getClk())
        {

            msgsnd(msq_id,&procs[snd_num],sizeof(struct processData),!IPC_NOWAIT);
            snd_num++;

        }

    }
    int stloc;
    waitpid(pid_s,&stloc,0);


    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    msgctl(msq_id,IPC_RMID,NULL);
    raise(SIGKILL);
}