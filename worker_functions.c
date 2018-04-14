#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
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

int getTotalLines(char* buf){
	int lines = 0;
	for(int i = 0; i < (int)(strlen(buf)); i++)
		if(buf[i] == '\n')
			lines++;
	return lines;
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

/* Initializes the map */
int initializeWorkerMap(worker_map** ptr, char* buffer, int size,int length){
	
	char* string = NULL;
	char* temp;
	int j = 0;
	for(int i = 0; i < size; i++){
		for(int turn = 0; j < length; turn++, j++){
			
			char c;
			strncpy(&c,&buffer[j],1);

			if(c == '\n'){
				temp = string;
				string = (char*)malloc((strlen(temp)+2)*sizeof(char));
				if(string == NULL)
					return WORKER_MEM_ERROR;
				
				strcpy(string,temp);
				strcat(string,&c);

				string[length] = '\0';

				ptr[0][i].dirID = i;
				ptr[0][i].dirPath = (char*)malloc((strlen(string)+1)*sizeof(char));
				if(ptr[0][i].dirPath == NULL)
					return WORKER_MEM_ERROR;

				strcpy(ptr[0][i].dirPath,string);

				printf("Inserted the string %s\n",string);

				free(string);
				free(temp);
				string = NULL;
				j++;
				break;
			}
			else{
				if(turn == 0){
					string = (char*)malloc(sizeof(char));
					if(string == NULL)
						return WORKER_MEM_ERROR;
					strncpy(string,&buffer[j],1);
				}
				else{
					temp = string;
					string = (char*)malloc((strlen(temp)+1)*sizeof(char));
					if(string == NULL)
						return -1;
					strcpy(string,temp);
					strcat(string,&c);
					free(temp);
				}
			}
		}
	}

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


/***********************/
/*** SIGNAL HANDLING ***/
/***********************/

void signal_handler(int signo){
	if(signo == SIGUSR2)
		printf("\n");
}