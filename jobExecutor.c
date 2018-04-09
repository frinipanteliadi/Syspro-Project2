#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
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
	

	/****************************/
	/*** CREATING THE WORKERS ***/
	/****************************/
	
	char buffer[sizeof(int)*4+1];
	char* pathname_read;
	char* pathname_write;
	pid_t pid;
	for(int i = 0; i < numWorkers; i++){
		
		sprintf(buffer,"%d",i);
		
		errorCode = allocatePathname(&pathname_read,&pathname_write,
			strlen(buffer));
		if(errorCode != OK){
			printErrorMessage(errorCode);
			return EXIT;
		}

		createPathname(&pathname_read,buffer,&pathname_write);				
		printf("\nPathnameRead: %s\n",pathname_read);
		printf("PathnameWrite: %s\n",pathname_write);

		if(mkfifo(pathname_read,0666) < 0){									// Creating a named-pipe where the jobExecutor reads from it
			printErrorMessage(PIPE_ERROR);
			return EXIT;
		}

		if(mkfifo(pathname_write,0666) < 0){								// Creating a named pipe where the jobExecutor writes to it
			printErrorMessage(PIPE_ERROR);
			return EXIT;
		}

		pid = fork();														// Creating a child process
		if(pid == -1){
			printErrorMessage(FORK_ERROR);
			return EXIT;
		}
		else if(pid == 0){													// Commands for the child process
			if(execlp("./worker","./worker",pathname_read,
				pathname_write,(char*)NULL) == -1){
				printErrorMessage(EXEC_ERROR);
				return EXIT;
			}
		}
		else{
			wait(NULL);
			printf("The parent reads from: %s\n",pathname_read);
			printf("The parent writes to: %s\n",pathname_write);
		}

		free(pathname_read);
		free(pathname_write);
	}

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***************************/

	deleteMap(&mapPtr,total_directories);
	fclose(fp);																// Closing the file
}