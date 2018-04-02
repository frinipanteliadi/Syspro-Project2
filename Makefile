all: jobExecutor

jobExecutor: jobExecutor.o functions.o
	gcc jobExecutor.o functions.o -o jobExecutor

jobExecutor.o: jobExecutor.c
	gcc -c jobExecutor.c

functions.o: functions.c
	gcc -c functions.c

clean:
	rm *.o jobExecutor
