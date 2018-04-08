#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "worker_functions.h"

int main(int argc, char* argv[]){
	int errorCode;
	if(errorCode = workerArgs(argc) != OK){
		printWorkerError(errorCode);
		return EXIT;
	}

	printf("(Child) Name of the pipe: %s\n",argv[1]);
	printf("I am the child process\n");
	return 0;
}