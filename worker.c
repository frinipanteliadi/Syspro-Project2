#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "worker_functions.h"

#define SIZE 1224

int main(int argc, char* argv[]){

	printf("\nCHILD STARTED\n");

	int errorCode;
	if(errorCode = workerArgs(argc) != OK){
		printWorkerError(errorCode);
		return EXIT;
	}

	char* pathname_read = argv[2];		
	char* pathname_write = argv[1];								
	/*printf("(Child)Reads from: %s\n",argv[2]);
	printf("(Child)Writes to: %s\n",argv[1]);*/

	int fd = open(argv[2],O_RDONLY);
	if(fd < 0){
		printWorkerError(OPEN_ERROR);
		return EXIT;
	}

	/***************************/
	/*** WAIT FOR THE PARENT ***/
	/***     TO FINISH       ***/
	/***      WRITING        ***/
	/***************************/

	raise(SIGSTOP);													// Wait until the parent finishes writing

	char buf[SIZE];
	int bytes = read(fd,buf,sizeof(buf));
	printf("The child read %d bytes from the file:\n",bytes);
	
	kill(getppid(),SIGCONT);	

	close(fd);
	buf[bytes-1] = 0;

	printf("%s\n",buf);

	return OK;
}