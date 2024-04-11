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
#include <limits.h>
#include <fcntl.h>

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

int shmid1, shmid2;

typedef struct SM1
{
    int pid;                // process id
    int mi;                 // number of required pages
    int fi;                 // number of frames allocated
    int pagetable[5000][3]; // page table
    int totalpagefaults;
    int totalillegalaccess;
} SM1;
typedef struct message2
{
    long type;
    int pid;
    int semid;
} message2;

typedef struct message3
{
    long type;
    int pageorframe;
    int pid;
    int semid;
} message3;

void sighand(int signum)
{
    if (signum == SIGINT)
    {
        // printf("Caught");
        // print the per process stats
        SM1 *sm1 = (SM1 *)shmat(shmid1, NULL, 0);
        char buffer[1000] = {'\0'};
        int i = 0;
        int fd = open("result.txt", O_CREAT | O_WRONLY | O_APPEND, 0777);
        printf("\nFinal Statistics for Processes\n");
        snprintf(buffer, sizeof(buffer), "\nFinal Statistics for Processes\n");
        write(fd, buffer, strlen(buffer));
        memset(buffer, '\0', sizeof(buffer));

        while (sm1[i].pid != -5)
        {
            printf("Process %d, Pid: %d\nNumber of page faults = %d\nNumber of invalid page reference = %d\n\n\n", i + 1, sm1[i].pid, sm1[i].totalpagefaults, sm1[i].totalillegalaccess);
            snprintf(buffer, sizeof(buffer), "Process %d, Pid: %d\nNumber of page faults = %d\nNumber of invalid page reference = %d\n\n\n", i + 1, sm1[i].pid, sm1[i].totalpagefaults, sm1[i].totalillegalaccess);
            write(fd, buffer, strlen(buffer));
            memset(buffer, '\0', sizeof(buffer));
            i++;
        }
        // fflush(stdout);
        printf("The results are also stored in result.txt\n");
        printf("This window will be open for 30 secs\n");
        sleep(30);
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sighand);
    int timestamp = 0;

    struct sembuf pop = {0, -1, 0};
    struct sembuf vop = {0, 1, 0};
    int fd = open("result.txt", O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if (argc != 5)
    {
        printf("Usage: %s <Message Queue 2 ID> <Message Queue 3 ID> <Shared Memory 1 ID> <Shared Memory 2 ID>\n", argv[0]);
        exit(1);
    }

    int msgid2 = atoi(argv[1]);
    int msgid3 = atoi(argv[2]);
    shmid1 = atoi(argv[3]);
    shmid2 = atoi(argv[4]);

    message2 msg2;
    message3 msg3;

    msg2.type = 5;
    msg2.pid = getpid();
    msgsnd(msgid2, (void *)&msg2, sizeof(message2), 0);

    SM1 *sm1 = (SM1 *)shmat(shmid1, NULL, 0);
    int *sm2 = (int *)shmat(shmid2, NULL, 0);

    // key_t key = ftok("master.c", 6);
    // int semid3 = semget(key, 1, IPC_CREAT | 0666);
    char buffer[1000] = {'\0'};
    while (1)
    {
        // wait for process to come
        msgrcv(msgid3, (void *)&msg3, sizeof(message3), 3, 0);
        timestamp++;

        printf("Global Ordering - (Timestamp %d, Process %d, Page %d)\n", timestamp, msg3.pid, msg3.pageorframe);
        snprintf(buffer, sizeof(buffer), "Global Ordering - (Timestamp %d, Process %d, Page %d)\n", timestamp, msg3.pid, msg3.pageorframe);
        write(fd, buffer, strlen(buffer));
        memset(buffer, '\0', sizeof(buffer));
        // V(semid3);
        // check if the requested page is in the page table of the process with that pid
        int i = 0;
        while (sm1[i].pid != msg3.pid)
        {
            i++;
        }

        int page = msg3.pageorframe;
        if (page == -9)
        {
            // process is done
            // free the frames
            for (int j = 0; j < sm1[i].mi; j++)
            {
                if (sm1[i].pagetable[j][0] != -1)
                {
                    sm2[sm1[i].pagetable[j][0]] = 1;
                    sm1[i].pagetable[j][0] = -1;
                    sm1[i].pagetable[j][1] = 0;
                    sm1[i].pagetable[j][2] = INT_MAX;
                }
            }
            msg2.type = 2;
            msg2.pid = msg3.pid;
            msg2.semid = msg3.semid;

            msgsnd(msgid2, (void *)&msg2, sizeof(message2), 0);
        }
        else if ((sm1[i].pagetable[page][0] != -1) && (sm1[i].pagetable[page][1] == 1))
        {
            // page there in memory and valid, return frame number
            sm1[i].pagetable[page][2] = timestamp;
            msg3.pageorframe = sm1[i].pagetable[page][0];
            msg3.type = 4;
            msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);
        }
        else if (page >= sm1[i].mi)
        {
            // illegal page number
            // ask process to kill themselves
            msg3.pageorframe = -2;
            msg3.type = 4;
            msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);

            // update total illegal access
            sm1[i].totalillegalaccess++;

            printf("\t\tInvalid Page Reference - (Process %d, Page %d)\n", msg3.pid, page);
            snprintf(buffer, sizeof(buffer), "\t\tInvalid Page Reference - (Process %d, Page %d)\n", msg3.pid, page);
            write(fd, buffer, strlen(buffer));
            memset(buffer, '\0', sizeof(buffer));

            // free the frames
            for (int j = 0; j < sm1[i].mi; j++)
            {
                if (sm1[i].pagetable[j][0] != -1)
                {
                    sm2[sm1[i].pagetable[j][0]] = 1;
                    sm1[i].pagetable[j][0] = -1;
                    sm1[i].pagetable[j][1] = 0;
                    sm1[i].pagetable[j][2] = INT_MAX;
                }
            }
            msg2.type = 2;
            msg2.pid = msg3.pid;
            msg2.semid = msg3.semid;
            msgsnd(msgid2, (void *)&msg2, sizeof(message2), 0);
        }
        else
        {
            // page fault
            // ask process to wait

            // Page Fault Handler (PFH)
            // check if there is a free frame in sm2
            int j = 0;
            while (sm2[j] != -1)
            {
                if (sm2[j] == 1)
                {
                    sm2[j] = 0;
                    break;
                }
                j++;
            }

            if (sm2[j] == -1)
            {
                // no free frame
                // find the page with the least timestamp
                int min = INT_MAX;
                int minpage = -1;
                for (int k = 0; k < sm1[i].mi; k++)
                {
                    if (sm1[i].pagetable[k][2] < min)
                    {
                        min = sm1[i].pagetable[k][2];
                        minpage = k;
                    }
                }
                if (minpage == -1)
                {
                    // update total illegal access
                    sm1[i].totalillegalaccess++;

                    printf("\t\tInvalid Page Reference - (Process %d, Page %d)\n", msg3.pid, page);
                    snprintf(buffer, sizeof(buffer), "\t\tInvalid Page Reference - (Process %d, Page %d)\n", msg3.pid, page);
                    write(fd, buffer, strlen(buffer));
                    memset(buffer, '\0', sizeof(buffer));

                    // process has no frames and no free frames
                    // ask process to kill themselves
                    msg3.pageorframe = -2;
                    msg3.type = 4;
                    msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);

                    // usleep(25000);

                    // ask scheduler to schedule another process
                    msg2.type = 2;
                    msg2.pid = msg3.pid;
                    msg2.semid = msg3.semid;
                    msgsnd(msgid2, (void *)&msg2, sizeof(message2), 0);
                    continue;
                }

                printf("\tPage fault sequence - (Process %d, Page %d)\n", msg3.pid, page);
                snprintf(buffer, sizeof(buffer), "\tPage fault sequence - (Process %d, Page %d)\n", msg3.pid, page);
                write(fd, buffer, strlen(buffer));
                memset(buffer, '\0', sizeof(buffer));
                sm1[i].totalpagefaults++;

                msg3.pageorframe = -1;
                msg3.type = 4;
                msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);

                sm1[i].pagetable[minpage][1] = 0;
                sm1[i].pagetable[page][0] = sm1[i].pagetable[minpage][0];
                sm1[i].pagetable[page][1] = 1;
                sm1[i].pagetable[page][2] = timestamp;
                sm1[i].pagetable[minpage][2] = INT_MAX;

                msg2.type = 1;
                msg2.pid = msg3.pid;
                msg2.semid = msg3.semid;
                msgsnd(msgid2, (void *)&msg2, sizeof(message2), 0);
            }

            else
            {
                printf("\tPage fault sequence - (Process %d, Page %d)\n", msg3.pid, page);
                snprintf(buffer, sizeof(buffer), "\tPage fault sequence - (Process %d, Page %d)\n", msg3.pid, page);
                write(fd, buffer, strlen(buffer));
                memset(buffer, '\0', sizeof(buffer));
                sm1[i].totalpagefaults++;

                msg3.pageorframe = -1;
                msg3.type = 4;
                msgsnd(msgid3, (void *)&msg3, sizeof(message3), 0);

                // free frame found
                sm1[i].pagetable[page][0] = j;
                sm1[i].pagetable[page][1] = 1;
                sm1[i].pagetable[page][2] = timestamp;

                msg2.type = 1;
                msg2.pid = msg3.pid;
                msg2.semid = msg3.semid;
                // printf("\t\t\t MMU added msg2.type = 1, msg2.pid = %d msg2.semid=%d\n", msg2.pid, msg2.semid);
                msgsnd(msgid2, (void *)&msg2, sizeof(message2), 0);
            }
        }
        // P(semid3);
    }
}