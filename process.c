#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

typedef struct message1
{
    long type;
    int pid;
} message1;

int main(int argc, char *argv[])
{
    struct sembuf pop = {0, -1, 0};
    struct sembuf vop = {0, 1, 0};

    if (argc != 4)
    {
        printf("Usage: %s <Reference String> <Virtual Address Space size> <Physical Address Space size>\n", argv[0]);
        exit(1);
    }

    int msgid1 = atoi(argv[2]);
    int msgid3 = atoi(argv[3]);

    key_t key = ftok("master.c", 4);
    int semid = semget(key, 1, IPC_CREAT | 0666);

    int pid = getpid();

    message1 msg1;
    msg1.type = 1;
    msg1.pid = pid;

    // send pid to ready queue
    msgsnd(msgid1, (void *)&msg1, sizeof(message1), 0);

    // wait till scheduler signals to start
    P(semid);

    // get the reference string
    char *refstr = argv[1];
    int n = strlen(refstr);

    // send the reference string to the scheduler, one character at a time
    
    
}
