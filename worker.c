#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "worker_functions.h"


int main(int argc, char* argv[]){

	int errorCode;
	if(errorCode = workerArgs(argc) != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}

	char* pathname_read = argv[2];		
	char* pathname_write = argv[1];								
	/*printf("(Child)Reads from: %s\n",argv[2]);
	printf("(Child)Writes to: %s\n",argv[1]);*/

	int fd = open(argv[2],O_RDONLY);
	if(fd < 0){
		printWorkerError(WORKER_OPEN_ERROR);
		return WORKER_EXIT;
	}

	/***************************/
	/*** WAIT FOR THE PARENT ***/
	/***     TO FINISH       ***/
	/***      WRITING        ***/
	/***************************/

	raise(SIGSTOP);													// Wait until the parent finishes writing

	int filedesc = open("info", O_RDONLY);
	if(filedesc < 0){
		printWorkerError(WORKER_OPEN_ERROR);
		return WORKER_EXIT;
	} 

	ssize_t bytes;
	read(filedesc,&bytes,sizeof(ssize_t));
	close(filedesc);

	char* buf;
	buf = (char*)malloc(bytes*sizeof(char));
	if(buf == NULL){
		printWorkerError(WORKER_MEM_ERROR);
		return WORKER_EXIT;
	}

	read(fd,buf,bytes);
	kill(getppid(),SIGCONT);	
	close(fd);
	
	buf[bytes-1] = 0;

	printf("%s\n",buf);

	free(buf);
	return WORKER_OK;
}