#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "worker_functions.h"

int main(int argc, char* argv[]){
	printf("Hi\n");
	int errorCode;
	if(errorCode = workerArgs(argc) != OK){
		printWorkerError(errorCode);
		return EXIT;
	}

	char* pathname_read = argv[2];											// The named-pipe which the worker uses for reading
	char* pathname_write = argv[1];											// The named-pipe which the worker uses for writing
	
	// printf("The Worker reads from: %s\n",pathname_read);
	// printf("The Worker writes to: %s\n",pathname_write);

	// int fd[2];																// File descriptors for the named-pipes
	// fd[0] = open(pathname_read,O_RDONLY);
	// // Sprintf("fd[0] = %d\n",fd[0]);
	// char buffer[80];
	// read(fd[0],buffer,sizeof(buffer));
	// printf("The child read: %s\n",buffer);
	return 0;
}