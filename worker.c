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
		
	for(int i = 0; i < distr; i++){
		
		/* Get the name of the directory's path in 
		   the right form for scandir() */
		char* name;
		name = (char*)malloc((strlen(map_ptr[i].dirPath)+1)*sizeof(char));
		if(name == NULL)
			return WORKER_MEM_ERROR;
		strncpy(name,map_ptr[i].dirPath,strlen(map_ptr[i].dirPath)-1);
		name[strlen(map_ptr[i].dirPath)-1] = '/';
		name[strlen(map_ptr[i].dirPath)] = '\0';
		
		// printf("* Directory: %s\n",name);

		/* Scan the directory */
		struct dirent **entry;
		int n = scandir(name,&entry,NULL,alphasort);
		int actual_files = n-2;
		if(n < 0){
			printf("Failed to scan the directory %s\n",name);
			return -1;
		}
		map_ptr[i].total_files = actual_files;
		

		map_ptr[i].dirFiles = (file_info*)malloc(map_ptr[i].total_files*sizeof(file_info));
		if(map_ptr[i].dirFiles == NULL)
			return WORKER_MEM_ERROR;

		file_info* temp_ptr;
		temp_ptr = map_ptr[i].dirFiles;

		for(int k = 2, j = 0; k < n; k++, j++){
			temp_ptr[j].file_name = (char*)malloc((strlen(entry[k]->d_name)+1)*sizeof(char));
			if(temp_ptr[j].file_name == NULL)
			return WORKER_MEM_ERROR;
			strncpy(temp_ptr[j].file_name,entry[k]->d_name,strlen(entry[k]->d_name));
			temp_ptr[j].file_name[strlen(entry[k]->d_name)] = '\0'; 
		}

		/* We must release the memory that scandir() allocated */
		for(int j = 0; j < n; j++)
			free(entry[j]);
		free(entry);
		free(name);
	}

	printWorkerMap(&map_ptr,distr);
	
	for(int i = 0; i < distr; i++){
		printf("*dirPath: %s\n",map_ptr[i].dirPath);
		for(int j = 0; j < map_ptr[i].total_files; j++)
			printf(" -Name: %s\n",map_ptr[i].dirFiles[j].file_name);
	}

	/*******************/
	/*** TERMINATION ***/
	/*******************/

	for(int i = 0; i < distr; i++){
		free(map_ptr[i].dirPath);

		for(int j = 0; j < map_ptr[i].total_files; j++)
			free(map_ptr[i].dirFiles[j].file_name);

		free(map_ptr[i].dirFiles);
	}
	free(map_ptr);

	return WORKER_OK;
}