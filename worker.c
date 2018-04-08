#include <stdio.h>
#include <stdlib.h>
#include "worker_functions.h"

int main(int argc, char* argv[]){
	int errorCode;
	if(errorCode = workerArgs(argc) != OK){
		printWorkerError(errorCode);
		return EXIT;
	}

	int pipefd[2];
	pipefd[0] = atoi(argv[1]);
	pipefd[1] = atoi(argv[2]);

	printf("Child Read FD: %d\n",pipefd[0]);
	printf("Child Write FD: %d\n",pipefd[1]);
	printf("I am the child process\n");
	return 0;
}