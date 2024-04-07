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

typedef struct message2
{
    long type;
    int pid;
    int semid;
} message2;

int main(int argc, char *argv[])
{
    struct sembuf pop = {0, -1, 0};
    struct sembuf vop = {0, 1, 0};

    if (argc != 3)
    {
        printf("Usage: %s <Message Queue 1 ID> <Message Queue 2 ID>\n", argv[0]);
        exit(1);
    }

    int msgid1 = atoi(argv[1]);
    int msgid2 = atoi(argv[2]);
    // int k = atoi(argv[3]);

    key_t key = ftok("master.c", 4);
    // int semid = semget(key, 1, IPC_CREAT | 0666);

    key = ftok("master.c", 7);
    int semid4 = semget(key, 1, IPC_CREAT | 0666);

    int pid = getpid();

    message1 msg1;
    message2 msg2;

    // receive a message2 from master from with process numbers, of type 6
    msgrcv(msgid2, (void *)&msg2, sizeof(message2), 6, 0);
    int k = msg2.pid;

    while (k > 0)
    {
        // wait for processes to come
        usleep(2500);
        msgrcv(msgid1, (void *)&msg1, sizeof(message1), 0, 0);

        printf("\t\tScheduling process %d\n", msg1.pid);

        // signal process to start
        // printf("\t\t secheduler signaling process %d and semid = %d\n", msg1.pid, msg1.semid);
        V(msg1.semid);

        // wait for mmu
        msgrcv(msgid2, (void *)&msg2, sizeof(message2), 0, 0);

        // check the type of message
        if (msg2.type == 2)
        {
            printf("\t\tProcess %d terminated\n", msg2.pid);
            k--;
        }
        else if (msg2.type == 1)
        {
            printf("\t\tProcess %d added to end of queue\n", msg2.pid);
            msg1.pid = msg2.pid;
            msg1.semid = msg2.semid;
            msg1.type = 1;
            msgsnd(msgid1, (void *)&msg1, sizeof(message1), 0);
        }
    }

    // signal master on semid4
    V(semid4);

    return 0;
}