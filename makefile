all: master.c  mmu.c  process.c  sched.c
	gcc master.c -o master
	gcc mmu.c -o mmu
	gcc sched.c -o sched
	gcc process.c -o process

clean:
	rm master mmu sched process