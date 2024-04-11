# Virtual-Memory-Simulation

## Simulating Virtual Memory through (Pure) Demand Paging

## Executing the Programs

Use `make all` to compile all the files and generate the executables. Then run the master file using `./master`.
The user will be prompted to enter the number of processes, virtual address space size and the physical address space size. Note that the master will create all other processes and the user does not need to run any other file.

## Results

The MMU module runs on an xterm terminal printing all the page requests and the associated responses (frame number or page fault or illegal page reference). The xterm terminal is kept for 30 seconds after all processes have finished so that the user can see the results. The outputs are also written automatically to the `result.txt` file.

## List of Modules

### Master Module

Implemented in `master.c`. This module is responsible for taking in the input parameters (number of processes, maximum number of pages per process, maximum number of frames) and creating and initializing the necessary data structures as shared memory segments and message queues. Some semaphores are also created for synchronization between the master and the scheduler. It then forks off the scheduler and the MMU which facilitate the scheduling and memory management respectively.

After that it generates the reference string for each process and writes it to the shared memory segment. The reference string is basically a sequence of page numbers that the process will access. The reference string is generated randomly (according to the input parameters and the specifications in the assignment). Note that it can also corrupt the reference string by randomly changing some of the page numbers to simulate page faults. We have ensured a very low probability of this happening as mentioned in the assignment. (Can be changed by changing the value of `P` in `master.c`)

After this, it creates the processes in intervals of 250ms as specified in the assignment. Then the scheduling and memory management modules take over and the master waits for the scheduler and MMU to finish their work.

Finally, the scheduler informs the master that all processes have finished and then master kills the scheduler and MMU and detaches and removes the shared memory segments and message queues.

If the user sends a SIGINT signal to the master, it will kill the scheduler and MMU and detach and remove the shared memory segments and message queues.

Note: The MMU is run not directly by the master, but by the xterm terminal. Hence, we need its PID to send it the SIGTERM signal. This is done by the MMU writing an initial message to the message queue which the master reads and stores the PID of the MMU. This is of type 5 which is used for this purpose only.

Similarly, the master writes the number of processes to the message queue which the scheduler reads and stores. This is of type 6 which is used for this purpose only. The scheduler needs this as it then knows how many processes to wait for before informing the master that all processes have finished.

### Scheduler Module

Implemented in `scheduler.c`. This module is responsible for scheduling the processes. It reads the number of processes from the message queue and then waits for the processes to send their PID to the message queue. The scheduler then waits for the message on the top of the ready queue and then schedules that process. It then waits for a message from the mmu to know if the process is done or if it has got a page fault. For the latter case, the scheduler will add the process back into the ready queue. The scheduler will continue to do this until all processes have finished.
After all the processes have finished, the scheduler will inform the master that all processes have finished.

### MMU Module

Implemented in `mmu.c`. This module is responsible for managing the memory. It first sends its pid to the master process (  that master can kill it once all processes are done; master has no other way of knowing the pid of mmu if we use xterm). After this the mmu waits for a page request message from a process. It then checks if the page is in the process table of that process. If yes, then mmu responds with a message to the process telling the frame number of the page. If not, then mmu sends a message to process informing there is a page fault and also informs the scheduler to add this process to the end of queue.The mmu will use an LRU scheme to replace a victim page and use that frame for the requested frame. The mmu also looks for illegal page references and if there are any then informs the process and scheduler of the same. The process is then instructed to terminate itself and scheduler will not add this process to the queue again. There is also the special case where a process has a page fault but no free frames are available and the process also have no frames of its own to replace, in this case the mmu will inform the process and scheduler of the same and the process will terminate itself. The mmu will continue to do the above until all processes have finished.

### Process Module

Implemented in `process.c`. This module reads the reference string passes to it as a command line argument and sends the page requests to the mmu. But before this, the process first adds itself to the ready queue and waits for it to be scheduled by the scheduler. After the process is scheduled, it sends the page requests to the mmu. The process will continue to do this until all the pages in the reference string are accessed. If there is a page fault, the process will go into a wait and the scheduler will add it back to the end of the queue. The page fault is handled by the mmu and when the scheduler schedules the process again, the requested page is avalaible in the page table. The process will continue to do this until all the pages in the reference string are accessed. After this the process will inform the mmu that it is done (-9 message) and then terminates itself.
