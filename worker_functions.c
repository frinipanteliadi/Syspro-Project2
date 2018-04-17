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

/*****************************/
/*** DIRECTORIES FUNCTIONS ***/
/*****************************/

int getNumberOfFilesInDirs(int distr, worker_map* map_ptr){
	DIR* my_directory;
	int total_files = 0;
	for(int i = 0; i < distr; i++){

		char* path_name;
		path_name = (char*)malloc((strlen(map_ptr[i].dirPath)+1)*sizeof(char));
		if(path_name == NULL)
			return WORKER_MEM_ERROR;
		
		strncpy(path_name,map_ptr[i].dirPath,strlen(map_ptr[i].dirPath)-1);
		path_name[strlen(map_ptr[i].dirPath)-1] = '/';
		path_name[strlen(map_ptr[i].dirPath)] = '\0';

		my_directory = opendir(path_name);
		if(my_directory == NULL)
			return WORKER_EXIT;

		struct dirent *entry;
		entry = readdir(my_directory);
		while(entry != NULL){
			if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0){
				entry = readdir(my_directory);
				continue;
			}

			total_files++;
			entry = readdir(my_directory);
		}

		if(closedir(my_directory) < 0)
			return WORKER_DIR_CLOSE_ERROR;
		
		free(path_name);
	}
	
	return total_files;
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

int manageLines(int total_files,file_info** files_ptr, int distr, worker_map** map_ptr){
	
	DIR* my_directory;

	*files_ptr = (file_info*)malloc(total_files*sizeof(file_info));
	if(*files_ptr == NULL)
		return WORKER_MEM_ERROR;

	int j = 0;
	for(int i = 0; i < distr; i++){

		char* path_name;
		path_name = (char*)malloc((strlen(map_ptr[0][i].dirPath)+1)*sizeof(char));
		if(path_name == NULL)
			return WORKER_MEM_ERROR;

		strncpy(path_name,map_ptr[0][i].dirPath,strlen(map_ptr[0][i].dirPath)-1);
		path_name[strlen(map_ptr[0][i].dirPath)-1] = '/';
		path_name[strlen(map_ptr[0][i].dirPath)] = '\0';

		my_directory = opendir(path_name);
		if(my_directory == NULL)
			return WORKER_DIR_OPEN_ERROR;

		struct dirent *entry;
		entry = readdir(my_directory);
		while(entry != NULL){
			char* pathname;
			if((strcmp(entry->d_name,".") == 0) || strcmp(entry->d_name,"..") == 0){
				entry = readdir(my_directory);
				continue;
			}
			else{
				files_ptr[0][j].file_name = (char*)malloc(strlen(entry->d_name)+1);
				if(files_ptr[0][j].file_name == NULL)
					return WORKER_MEM_ERROR;
				strcpy(files_ptr[0][j].file_name,entry->d_name);

				files_ptr[0][j].file_location = (char*)malloc((strlen(path_name)+1)*sizeof(char));
				if(files_ptr[0][j].file_location == NULL)
					return WORKER_MEM_ERROR;
				strcpy(files_ptr[0][j].file_location,path_name);

				pathname = (char*)malloc((strlen(files_ptr[0][j].file_location)+strlen(files_ptr[0][j].file_name)+1)*sizeof(char));
				if(pathname == NULL)
					return WORKER_MEM_ERROR;

				strcpy(pathname,files_ptr[0][j].file_location);
				strcat(pathname,files_ptr[0][j].file_name);

				FILE* fp;
				fp = fopen(pathname,"r");
				if(fp == NULL)
					return WORKER_OPEN_ERROR;

				files_ptr[0][j].total_lines = getNumberOfLines(fp);

				files_ptr[0][j].file_lines = (line*)malloc(files_ptr[0][j].total_lines*sizeof(line));
				if(files_ptr[0][j].file_lines == NULL)
					return WORKER_MEM_ERROR;

				rewind(fp);

				char* line = NULL;
				size_t n = 0;
				int k = 0;
				while(getline(&line,&n,fp) != -1){
					files_ptr[0][j].file_lines[k].line_content = (char*)malloc((strlen(line)+1)*sizeof(char));
					if(files_ptr[0][j].file_lines[k].line_content == NULL)
						return WORKER_MEM_ERROR;
					strcpy(files_ptr[0][j].file_lines[k].line_content,line);

					files_ptr[0][j].file_lines[k].line_id = k;
					k++;
				}

				free(line);

				if(fclose(fp) != 0)
					return -1;

				j++;
				entry = readdir(my_directory);
			}

			memset(pathname,'\0',strlen(pathname)+1);
			free(pathname);
			pathname = NULL;
		}

		if(closedir(my_directory) < 0)
			return WORKER_DIR_CLOSE_ERROR;
		free(path_name);
	}

	return WORKER_OK;
}

void printLinesStruct(file_info** files_ptr, int total_files){
	for(int i = 0; i < total_files; i++){
		printf("************************\n");
		printf("* Location: %s\n",files_ptr[0][i].file_location);
		printf("* Name: %s\n",files_ptr[0][i].file_name);
		printf("* Total Lines: %d\n",files_ptr[0][i].total_lines);

		for(int j = 0; j < files_ptr[0][i].total_lines; j++){
			printf("* LineID: %d\n",files_ptr[0][i].file_lines[j].line_id);
			printf("* Line: %s\n",files_ptr[0][i].file_lines[j].line_content);
		}
		printf("************************\n");
	}
}