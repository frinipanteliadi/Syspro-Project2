#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "worker_functions.h"

int main(int argc, char* argv[]){

	int errorCode;

	if(errorCode = workerArgs(argc) != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}

	/****************************/
	/*** GETTING THE NAMES OF ***/ 
	/***      THE PIPES       ***/
	/****************************/

	char* pathname_read = argv[2];		
	int fd_read;
	
	char* pathname_write = argv[1];	
	int fd_write;
	
	errorCode = openingPipes(pathname_read, pathname_write,
		&fd_read,&fd_write);
	if(errorCode != WORKER_OK){
		printWorkerError(errorCode);
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

	// printf("** WORKER(%d) **\n",worker_no);							
	
	/********************************/
	/*** FETCHING THE DIRECTORIES ***/
	/***       VIA PIPES          ***/ 
	/********************************/

	char length[1024];
	int string_length;
	for(int i = 0; i < distr; i++){
		
		read(fd_read,length,1024);												// Read the length of the path
		length[strlen(length)]='\0';
		string_length = atoi(length);

		// printf("(Child) Length %s\n",length);
		write(fd_write, "OK", strlen("OK"));
			
		errorCode = initializeWorkerMap(&map_ptr,i,fd_read,string_length);
		if(errorCode != WORKER_OK){
			printWorkerError(errorCode);
			return WORKER_EXIT;
		}

		write(fd_write,"OK",strlen("OK"));
	}
	
	// printWorkerMap(&map_ptr,distr);
	
	/****************************/
	/*** RETRIEVING THE FILES ***/
	/***        FROM          ***/
	/***   THE DIRECTORIES    ***/
	/****************************/
	
	errorCode = fileInformation	(distr,&map_ptr);
	if(errorCode != WORKER_OK)
		return -1;

	printWorkerMap(&map_ptr,distr);
	
	for(int i = 0; i < distr; i++){
		printf("\n\n");
		printf("*dirPath: %s",map_ptr[i].dirPath);
		for(int j = 0; j < map_ptr[i].total_files; j++){
			printf(" -Name: %s\n",map_ptr[i].dirFiles[j].file_name);
			printf(" -Path: %s\n",map_ptr[i].dirFiles[j].full_path);
		}
	}

	/*******************/
	/*** TERMINATION ***/
	/*******************/

	for(int i = 0; i < distr; i++){
		free(map_ptr[i].dirPath);

		for(int j = 0; j < map_ptr[i].total_files; j++){
			free(map_ptr[i].dirFiles[j].file_name);
			free(map_ptr[i].dirFiles[j].full_path);
		}

		free(map_ptr[i].dirFiles);
	}
	free(map_ptr);

	return WORKER_OK;
}