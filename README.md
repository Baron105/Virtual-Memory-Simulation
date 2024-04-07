# Virtual-Memory-Simulation

## Simulating Virtual Memory through (Pure) Demand Paging

## List of Modules

### Master Module

Implemented in `master.c`. This module is responsible for taking in the input parameters (number of processes, maximum number of pages per process, maximum number of frames) and creating and initializing the necessary data structures as shared memory segments and message queues. Some semaphores are also created for synchronization between the master and the scheduler. It then forks off the scheduler and the MMU which facilitate the scheduling and memory management respectively.

After that it generates the reference string for each process and writes it to the shared memory segment. The reference string is basically a sequence of page numbers that the process will access. The reference string is generated randomly (according to the input parameters and the specifications in the assignment). Note that it can also corrupt the reference string by randomly changing some of the page numbers to simulate page faults. We have ensured a very low probability of this happening as mentioned in the assignment.

After this, it creates the processes in intervals of 250ms as specified in the assignment. Then the scheduling and memory management modules take over and the master waits for the scheduler and MMU to finish their work.

Finally, the scheduler informs the master that all processes have finished and then master kills the scheduler and MMU and detaches and removes the shared memory segments and message queues.

Note: The MMU is run not directly by the master, but by the xterm terminal. Hence, we need its PID to kill 

### Scheduler Module

