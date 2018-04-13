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

	struct sigaction act;
	sigemptyset(&act.sa_mask);
	// sigaddset(&act.sa_mask, SIGINT);
	act.sa_handler = signal_handler;
	act.sa_flags = SA_SIGINFO;
	if(sigaction(SIGUSR2, &act, NULL) < 0){
		printf("sigaction failed\n");
		return WORKER_EXIT;
	}


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


	/*****************************/
	/*** FETCH THE DIRECTORIES ***/
	/*****************************/

	pause();
	int filedesc = open("info", O_RDONLY);
	if(filedesc < 0){
		printWorkerError(WORKER_OPEN_ERROR);
		return WORKER_EXIT;
	} 

	ssize_t bytes;
	read(filedesc,&bytes,sizeof(ssize_t));
	close(filedesc);

	// printf("The child found out that the parent");
	// printf(" wrote %d bytes to the file\n",bytes);

	char* buf;
	buf = (char*)malloc(bytes);
	if(buf == NULL){
		printWorkerError(WORKER_MEM_ERROR);
		return WORKER_EXIT;
	}


	ssize_t read_so_far = 0;
	while(read_so_far != bytes)
		read_so_far += read(fd,buf,(bytes-read_so_far));
	

	close(fd);
	kill(getppid(),SIGUSR1);	
	

	// printf("Read %d bytes\n",read_so_far);
	printf("Worker:\n");
	for(int i = 0; i < (int)bytes; i++)
		putchar(buf[i]);	
	// printf("Length: %d\n",(int)(strlen(buf)));

	free(buf);
	return WORKER_OK;
}