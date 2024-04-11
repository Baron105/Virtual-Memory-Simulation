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
    int semid;
} message1;

typedef struct message3
{
    long type;
    int pageorframe;
    int pid;
    int semid;
} message3;

int main(int argc, char *argv[])
{
    // printf("Process has started\n");

    struct sembuf pop = {0, -1, 0};
    struct sembuf vop = {0, 1, 0};

    if (argc != 4)
    {
        printf("Usage: %s <Reference String> <Virtual Address Space size> <Physical Address Space size>\n", argv[0]);
        exit(1);
    }

    int msgid1 = atoi(argv[2]);
    int msgid3 = atoi(argv[3]);

    char refstr[100000] = {'\0'};
    strcpy(refstr, argv[1]);
    // printf("%s   pid=%d\n", refstr, getpid());

    key_t key = ftok("master.c", 4);
    // int semid = semget(key, 1, IPC_CREAT | 0666);

    // key = ftok("master.c", 6);
    // int semid3 = semget(key, 1, IPC_CREAT | 0666);

    // generate a random key for the process
    key = ftok("process.c", getpid() % 255);
    int semaphoreid = semget(key, 1, IPC_CREAT | 0666);

    int pid = getpid();

    message1 msg1;
    msg1.type = 1;
    msg1.pid = pid;
    msg1.semid = semaphoreid;

    // send pid to ready queue
    msgsnd(msgid1, (void *)&msg1, sizeof(message1), 0);

    // wait till scheduler signals to start
    // printf("pid = %d\n", pid);
    P(semaphoreid);
    // printf("Process %d: Started with semaphore id =%d\n", pid, semaphoreid);

    // send the reference string to the scheduler, one character at a time
    int i = 0;
    while (refstr[i] != '\0')
    {
        message3 msg3;
        msg3.type = 3;
        msg3.pid = pid;
        msg3.semid = semaphoreid;
        int j = 0;
        char temp[10] = {'\0'};
        int k = 0;
        // extract the page number from the reference string going character by character
        int x = i;
        while (refstr[i] != '.' && refstr[i] != '\0')
        {
            temp[k] = refstr[i];
            k++;
            i++;
        }
        j = atoi(temp);
        // printf("process id = %d, page number = %d\n", pid, j);
        i++;
        msg3.pageorframe = j;
        msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);
        // P(semid3);
        // wait for the mmu to allocate the frame
        msgrcv(msgid3, (void *)&msg3, sizeof(message3), 4, 0);
        // V(semid3);
        // check the validity of the frame number
        if (msg3.pageorframe == -2)
        {
            printf("Process %d: ", pid);
            printf("Invalid Page Number, Terminating\n");
            // delete the semaphore
            semctl(semaphoreid, 0, IPC_RMID);
            exit(1);
        }
        else if (msg3.pageorframe == -1)
        {
            // printf("Process %d: ", pid);
            // printf("Page Fault\nWaiting for page to be loaded\n");
            // wait for the page to be loaded
            // scheduler will signal when the page is loaded
            i = x;
            P(semaphoreid);
            continue;
        }
        else
        {
            // printf("Process %d: ", pid);
            // printf("Frame %d allocated\n", msg3.pageorframe);
        }
    }

    // send the termination signal to the mmu
    printf("Process %d: ", pid);
    printf("Got all frames properly");
    message3 msg3;
    msg3.type = 3;
    msg3.pid = pid;
    msg3.pageorframe = -9;
    msg3.semid = semaphoreid;

    msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);
    printf(", Terminating\n");

    // delete the semaphore
    semctl(semaphoreid, 0, IPC_RMID);

    return 0;
}
