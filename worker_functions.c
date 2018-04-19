#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "worker_functions.h"

/**********************/
/*** MISC FUNCTIONS ***/
/**********************/

int compareKeys(char* a, char* b){
	return((*a)-(*b));
} 

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
		case WORKER_DIR_OPEN_ERROR:
			printf("Failed to open a directory\n");
			break;
		case WORKER_DIR_CLOSE_ERROR:
			printf("Failed to close a directory\n");
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

	ptr[0][i].total_files = 0;
	ptr[0][i].dirFiles = NULL;

	return WORKER_OK;
}

/* Prints the map (Helpful for debugging) */
void printWorkerMap(worker_map** ptr, int size){
	for(int i = 0; i < size; i++){
		printf("**************************\n");
		printf("*dirID: %d\n",ptr[0][i].dirID);
		printf("*dirPath: %s",ptr[0][i].dirPath);
		// printf("*dirFiles: %d\n",ptr[0][i].dirFiles);
		printf("*Total Files: %d\n",ptr[0][i].total_files);
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

int getNumberOfLines(FILE* fp){
	size_t n = 0;
	int lines = 0;
	char* lineptr = NULL;
	while(getline(&lineptr,&n,fp)!=-1){
		if(strlen(lineptr) == 1)
			continue;			
		lines++;
	}

	free(lineptr);
	return lines;
}

int fileInformation(int distr, worker_map** map_ptr){
	
	for(int i = 0; i < distr; i++){

		/* Creating the full path of the directory
		   for scandir() */
		char* name;
		name = (char*)malloc((strlen(map_ptr[0][i].dirPath)+1)*sizeof(char));
		if(name == NULL)
			return WORKER_MEM_ERROR;
		strncpy(name,map_ptr[0][i].dirPath,strlen(map_ptr[0][i].dirPath)-1);
		name[strlen(map_ptr[0][i].dirPath)-1] = '/';
		name[strlen(map_ptr[0][i].dirPath)] = '\0';

		/* Scan the directory */
		struct dirent **entry;
		int n = scandir(name,&entry,NULL,alphasort);
		if(n < 0){
			printf("Failed to scan the directory: %s\n",name);
			return -1;
		}
		
		/* Remove both the current and the parent directory */
		int actual_files = n-2;
		map_ptr[0][i].total_files = actual_files;

		/* Allocate memory for the structure */
		map_ptr[0][i].dirFiles = (file_info*)malloc(map_ptr[0][i].total_files*sizeof(file_info));
		if(map_ptr[0][i].dirFiles == NULL)
			return WORKER_MEM_ERROR;

		file_info* temp_ptr;
		temp_ptr = map_ptr[0][i].dirFiles;

		/* Add the name of each file and its full path */
		for(int k = 2, j = 0; k < n; k++, j++){
			temp_ptr[j].file_name = (char*)malloc((strlen(entry[k]->d_name)+1)*sizeof(char));
			if(temp_ptr[j].file_name == NULL)
				return WORKER_MEM_ERROR;
			strncpy(temp_ptr[j].file_name,entry[k]->d_name,strlen(entry[k]->d_name));
			temp_ptr[j].file_name[strlen(entry[k]->d_name)] = '\0';

			temp_ptr[j].full_path = (char*)malloc((strlen(name)+strlen(entry[k]->d_name)+1)*sizeof(char));
			if(temp_ptr[j].full_path == NULL)
				return WORKER_MEM_ERROR;
			strcpy(temp_ptr[j].full_path,name);
			strcat(temp_ptr[j].full_path,entry[k]->d_name);
		}

		/* Release the memory that scandir() allocated */
		for(int j = 0; j < n; j++)
			free(entry[j]);
		free(entry);
	
		free(name);
	}
	return WORKER_OK;
}