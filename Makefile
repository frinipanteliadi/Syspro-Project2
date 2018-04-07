all: jobExecutor worker

jobExecutor: jobExecutor.o functions.o
	gcc jobExecutor.o functions.o -o jobExecutor

jobExecutor.o: jobExecutor.c
	gcc -c jobExecutor.c

functions.o: functions.c
	gcc -c functions.c

worker: worker.o
	gcc worker.o -o worker

worker.o: worker.c
	gcc -c worker.c

clean:
	rm -rf *.o
	rm -rf jobExecutor
	rm -rf worker
