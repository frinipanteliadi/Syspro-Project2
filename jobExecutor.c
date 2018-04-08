#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "functions.h"

//   Commands for terminal:
// ./jobExecutor -d docfile
// ./jobExecutor -d docfile -w numWorkers
// ./jobExecutor -w numWorkers -d docfile

int main(int argc, char* argv[]){
	
	/****************************/
	/*** HANDLING THE COMMAND ***/ 
	/***   LINE ARGUMENTS     ***/
	/****************************/
	int errorCode = CommandLineArg(argc);									// Checking the number of arguments that were provided
	if(errorCode != OK){
		printErrorMessage(errorCode);
		return EXIT;
	}

	int numWorkers = 10;													// Default value for the number of Worker processes
	FILE* fp;					

	for(int i = 1; i < argc; i+=2){											// The arguments can be provided in different ways
		if(strcmp(argv[i],"-d") == 0){
			fp = fopen(argv[i+1],"r");		
			if(fp == NULL){
				printErrorMessage(FILE_NOT_OPEN);
				return EXIT;
			}
		}
		else if(strcmp(argv[i],"-w") == 0)
			numWorkers = atoi(argv[i+1]);
	}

	printf("NumWorkers: %d\n",numWorkers);

	int total_directories = getNumberOfLines(fp);
	rewind(fp);																// Return to the beggining of the file
	
	/*********************************/
	/*** CREATING AND INITIALIZING ***/
	/***         THE MAP           ***/
	/*********************************/

	map* mapPtr;
	errorCode = createMap(&mapPtr,total_directories);
	if(errorCode != 0){
		printf("An error occured while trying to create the Map\n");
		printErrorMessage(errorCode);
		return EXIT;
	}

	errorCode = initializeMap(fp,mapPtr,total_directories);
	if(errorCode != 0){
		printf("An error occured while trying to");
		printf(" initialize the Map\n");
		printErrorMessage(errorCode);
		return EXIT;
	}

	// printMap(mapPtr,total_directories);
	
	int** pipesPtr;
	errorCode = createPipePtr(&pipesPtr,numWorkers);
	if(errorCode != OK){
		printErrorMessage(errorCode);
		return EXIT;
	}


	char str[1224];
	char str1[1224];

	/****************************/
	/*** CREATING THE WORKERS ***/
	/****************************/

	pid_t pid;
	for(int i = 0; i < numWorkers; i++){
		
		errorCode = createPipe(&pipesPtr[i]);
		if(errorCode != OK){
			printErrorMessage(errorCode);
			return EXIT;
		}

		if(pipe(pipesPtr[i]) == -1){
			printErrorMessage(PIPE_ERROR);
			return EXIT;
		}

		pid = fork();														// Create a child process
		if(pid == -1){
			printErrorMessage(FORK_ERROR);
			return EXIT;
		}
		else if(pid == 0){
			sprintf(str,"%d",pipesPtr[i][0]);
			sprintf(str1,"%d",pipesPtr[i][1]);
			if(execlp("./worker","./worker",str,str1,(char*)NULL) == -1){
				printErrorMessage(EXEC_ERROR);
				return EXIT;
			}
		}
		else{
			wait(NULL);
			printf("Parent Read FD: %d\n",pipesPtr[i][0]);
			printf("Parent Write FD: %d\n",pipesPtr[i][1]);
			printf("I am the parent\n");
		}
	}

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***************************/

	deleteMap(&mapPtr,total_directories);
	deletePipe(&pipesPtr,numWorkers);
	fclose(fp);																// Closing the file
}