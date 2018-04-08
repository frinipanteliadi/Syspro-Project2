#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "functions.h"

/* Prints an error message according to an error code */
void printErrorMessage(int errorCode){
	switch(errorCode){
		case MEM_ERROR:
			printf("Attempt to allocate memory was unsuccessful");
			break;
		case FEW_ARG:
			printf("Too few arguments were provided");
			printf(" in the command line\n");
			break;
		case NOT_ENOUGH_ARG:
			printf("The number of arguments provided in ");
			printf("the command line is invalid\n");
			break;
		case MANY_ARG:
			printf("Too many arguments were provided");
			printf(" in the command line\n");
			break;
		case FILE_NOT_OPEN:
			printf("Attempt to open the given file");
			printf(" was unsuccessful\n");
			break;
		case FORK_ERROR:
			printf("Attempt to create a child process");
			printf(" was unsuccessful\n");
			break;
		case PIPE_ERROR:
			printf("Attempt to create a pipeline");
			printf(" was unsuccessful\n");
			break;
		case EXEC_ERROR:
			printf("The exec system call failed\n");
			break;
	}
}


/***************************/
/*** ASSISTIVE FUNCTIONS ***/
/***************************/

/* Checks the number of arguments that were provided */
int CommandLineArg(int arguments){
	if(arguments < 3)
		return FEW_ARG;

	else if(arguments > 3 && arguments < 5)
		return NOT_ENOUGH_ARG;
	
	else if(arguments > 5)
		return MANY_ARG;
	
	return OK;
}

/* Returns the total number of lines of a file */
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

/*********************/
/*** MAP FUNCTIONS ***/
/*********************/

/* Allocates memory for the Map */
int createMap(map** ptr,int size){
	*ptr = (map*)malloc(size*sizeof(map));
	if(*ptr == NULL)
		return MEM_ERROR;
	
	return OK;
}

/* Passes the paths for the directories from the file into the Map */
int initializeMap(FILE* fp, map* array,int lines){
	size_t n = 0;
	int i = 0;
	char *line = NULL;
	while(getline(&line,&n,fp)!=-1){
		if(i == lines)
			break;
		if(strlen(line) == 1)
			continue;
		int id = i;		
		array[i].dirID = id;
		array[i].dirPath = (char*)malloc(strlen(line)+1);
		if(array[i].dirPath == NULL)
			return MEM_ERROR;
		strcpy(array[i].dirPath,line);
		i++;
	}

	free(line);
	return OK;
}

/* Prints the contents of the Map */
void printMap(map* ptr, int size){
	for(int i = 0; i < size; i++){
		printf("************************\n");
		printf("*dirID: %d\n",ptr[i].dirID);
		printf("*dirPath: %s\n",ptr[i].dirPath);
		printf("************************\n");
	}
}

/* Deallocates all of the memory the Map holds */
void deleteMap(map** ptr, int size){
	for(int i = 0; i < size; i++)
		free((*ptr)[i].dirPath);
	free(*ptr);
}

/**********************/
/*** PIPE FUNCTIONS ***/
/**********************/

/* Creates an array of pointers which point to the array
   that holds the file descriptors of a pipe */
int createPipePtr(int*** ptr, int size){
	*ptr = (int**)malloc(size*sizeof(int*));
	if(*ptr == NULL)
		return MEM_ERROR;

	return OK;
}

/* Creates the array which holds the file descriptors 
   of a pipe */
int createPipe(int** ptr){
	*ptr = (int*)malloc(2*sizeof(int));
	if(*ptr == NULL)
		return MEM_ERROR;

	return OK;
}

/* Deallocates all the memory that was associated with
   pipes */
void deletePipe(int*** ptr, int size){
	for(int i = 0; i < size; i++)
		free(ptr[0][i]);
	
	free(ptr[0]);
}

int allocatePathname(char** ptr,int length){
	*ptr = (char*)malloc(strlen("Pipe")+length+1);
	if(*ptr == NULL)
		return MEM_ERROR;
	return OK;
}

void createPathname(char** ptr, char* buffer){
	strcpy(*ptr,"Pipe");
	strcat(*ptr,buffer);
}
