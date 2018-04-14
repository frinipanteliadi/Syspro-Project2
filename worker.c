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
	int fd_read = open(argv[2],O_RDONLY);
	if(fd_read < 0){
		printWorkerError(WORKER_OPEN_ERROR);
		return WORKER_EXIT;
	}
	
	char* pathname_write = argv[1];	
	int fd_write = open(argv[1], O_WRONLY);
	if(fd_write < 0){
		printWorkerError(WORKER_OPEN_ERROR);
		return WORKER_EXIT;
	}
	
	/*printf("(Child)Reads from: %s\n",argv[2]);
	printf("(Child)Writes to: %s\n",argv[1]);*/
	
	int worker_no = atoi(argv[3]);											// Worker's number
	int distr = atoi(argv[4]);												// Number of paths for the worker

	worker_map *map_ptr;
	errorCode = createWorkerMap(&map_ptr,distr);
	if(errorCode != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}

	printf("** WORKER(%d) **\n",worker_no);							
	
	char length[1024];
	int string_length;
	for(int i = 0; i < distr; i++){
		
		read(fd_read,length,1024);												// Read the length of the path
		length[strlen(length)]='\0';
		string_length = atoi(length);

		// printf("(Child) Length %s\n",length);
		write(fd_write, "OK", strlen("OK"));
			

		/*char* buffer;
		buffer = (char*)malloc((string_length+1)*sizeof(char));
		if(buffer == NULL)
			return WORKER_MEM_ERROR;*/

		map_ptr[i].dirID = i;
		map_ptr[i].dirPath = (char*)malloc((string_length+1)*sizeof(char));
		if(map_ptr[i].dirPath == NULL)
			return WORKER_MEM_ERROR;

		read(fd_read,map_ptr[i].dirPath,(size_t)string_length*sizeof(char));
		map_ptr[i].dirPath[string_length] = '\0';
		


		// read(fd_read,buffer,(size_t)string_length*sizeof(char));
		// buffer[string_length] = '\0';
		write(fd_write,"OK",strlen("OK"));

		// printf("buffer %s\n",buffer);
		// free(buffer);
	}
	
	printWorkerMap(&map_ptr,distr);

	close(fd_read);
	close(fd_write);
	
	deleteWorkerMap(&map_ptr,distr);
	return WORKER_OK;
}