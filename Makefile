all: jobExecutor worker

jobExecutor: jobExecutor.o functions.o
	gcc jobExecutor.o functions.o -o jobExecutor

jobExecutor.o: jobExecutor.c
	gcc -c -g jobExecutor.c

functions.o: functions.c
	gcc -c -g functions.c


worker: worker.o worker_functions.o
	gcc worker.o worker_functions.o -o worker

worker.o: worker.c
	gcc -c -g worker.c

worker_functions.o: worker_functions.c
	gcc -c -g worker_functions.c

clean:
	rm -rf *.txt
	rm -rf *.o
	rm -rf jobExecutor
	rm -rf worker
	rm -rf info
	find .  -maxdepth 1 -type p -delete
