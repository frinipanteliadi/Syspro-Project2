#include <stdio.h>
#include "worker_functions.h"

/* Prints the message that corresponds to
   the error code */
void printWorkerError(int errorCode){
	switch(errorCode){
		case WORKER_ARGS_ERROR:
			printf("The number of arguments that ");
			printf("were provided for the worker ");
			printf("is invalid\n");
			break;
		case WORKER_RAISE_ERROR:
			printf("The raise() function");
			printf(" failed\n");
			break;
		case WORKER_OPEN_ERROR:
			printf("Attempt to open the file");
			printf(" (named-pipe) was unsuccessful\n");
			break;
		case WORKER_MEM_ERROR:
			printf("Failed to allocate memory\n");
			break;
	}
}

/* Checks the number of arguments that 
   were provided */ 
int workerArgs(int arguments){
	if(arguments != 3)
		return WORKER_ARGS_ERROR;
	return WORKER_OK;
}