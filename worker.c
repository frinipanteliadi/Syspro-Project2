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

	char* pathname_read = argv[2];											// The named-pipe which the worker uses for reading
	char* pathname_write = argv[1];											// The named-pipe which the worker uses for writing
	
	printf("\nThe Worker reads from: %s\n",pathname_read);
	printf("The Worker writes to: %s\n",pathname_write);
	return 0;
}