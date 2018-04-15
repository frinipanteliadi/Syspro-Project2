#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "worker_functions.h"

/**********************/
/*** MISC FUNCTIONS ***/
/**********************/

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
	if(arguments != 5)
		return WORKER_ARGS_ERROR;
	return WORKER_OK;
}

/* Frees all of the memory that we allocated during 
   execution and close all of the files */
void freeingMemory(int* fd_read, int* fd_write, worker_map** ptr, int distr){
	close(*fd_read);
	close(*fd_write);
	deleteWorkerMap(ptr,distr);
}

/*********************/
/*** MAP FUNCTIONS ***/
/*********************/

/* Allocotes memory for storing the map */
int createWorkerMap(worker_map** ptr, int size){
	*ptr = (worker_map*)malloc(size*sizeof(worker_map));
	if(*ptr == NULL)
		return WORKER_MEM_ERROR;
	return WORKER_OK;
}

/* Initializes the i-th member of the Map */
int initializeWorkerMap(worker_map** ptr, int i, int fd, int string_length){
	ptr[0][i].dirID = i;
	ptr[0][i].dirPath = (char*)malloc((string_length+1)*sizeof(char));
	if(ptr[0][i].dirPath == NULL)
		return WORKER_MEM_ERROR;

	read(fd,ptr[0][i].dirPath,(size_t)string_length*sizeof(char));
	ptr[0][i].dirPath[string_length] = '\0';

	return WORKER_OK;
}

/* Prints the map (Helpful for debugging) */
void printWorkerMap(worker_map** ptr, int size){
	for(int i = 0; i < size; i++){
		printf("**************************\n");
		printf("*dirID: %d\n",ptr[0][i].dirID);
		printf("*dirPath: %s\n",ptr[0][i].dirPath);
		printf("**************************\n");
	}
}

/* Deletes all memory associated with the map */
void deleteWorkerMap(worker_map** ptr, int size){
	for(int i = 0; i < size; i++)
		free(ptr[0][i].dirPath);
	free(*ptr);
}


/**********************/
/*** PIPE FUNCTIONS ***/
/**********************/

/* Opens the pipes for reading and writing */
int openingPipes(char* reading_pipe, char* writing_pipe, 
	int* fd_read, int* fd_write){

	*fd_read = open(reading_pipe, O_RDONLY);
	if(*fd_read < 0)
		return WORKER_OPEN_ERROR;

	*fd_write = open(writing_pipe, O_WRONLY);
	if(*fd_write < 0)
		return WORKER_OPEN_ERROR;

	return WORKER_OK;
}

