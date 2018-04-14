#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
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
	
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	// sigaddset(&act.sa_mask, SIGINT);
	act.sa_handler = signal_handler;
	act.sa_flags = SA_SIGINFO;
	if(sigaction(SIGUSR1,&act,NULL) < 0){
		printf("sigaction failed\n");
		return EXIT;
	}

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

	// printf("NumWorkers: %d\n",numWorkers);

	int total_directories = getNumberOfLines(fp);							// Total number of paths kept in the file
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

	fclose(fp);																// Closing the file

	// printMap(mapPtr,total_directories);
	

	/****************************/
	/*** CREATING THE WORKERS ***/
	/****************************/
	/***  CREATING THE PIPES  ***/
	/****************************/
	/***   DISTRIBUTING THE   ***/
	/***     DIRECTORIES      ***/
	/****************************/
	
	pipes* pipes_ptr;
	errorCode = allocatePipeArray(&pipes_ptr,numWorkers);
	if(errorCode != OK){
		printErrorMessage(errorCode);
		return EXIT;
	}

	char buffer[sizeof(int)*4+1];
	char worker_distr[sizeof(int)*4+1];
	char* pathname_read;
	char* pathname_write;
	int fd;
	pid_t pid;
	int distr[numWorkers];
	int current_map_position = 0;


	setDistributions(total_directories,numWorkers,distr);
	/*for(int i = 0; i < numWorkers; i++)
		printf("Worker%d: %d directories\n",i,distr[i]);*/


	for(int i = 0; i < numWorkers; i++){
		sprintf(buffer,"%d",i);
		sprintf(worker_distr,"%d",distr[i]);

		errorCode = allocatePathname(&pathname_read,&pathname_write,
			strlen(buffer));
		if(errorCode != OK){
			printErrorMessage(errorCode);
			return EXIT;
		}

		createPathname(&pathname_read,buffer,&pathname_write);				
		
		errorCode = initializePipeArray(&pipes_ptr,i,
			pathname_read,pathname_write);
		if(errorCode != OK){
			printErrorMessage(errorCode);
			return EXIT;
		}

		/*printf("(Parent)Reads from: %s\n",pathname_read);
		printf("(Parent)Writes to: %s\n",pathname_write);*/
		
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
				pathname_write,buffer,
				worker_distr,(char*)NULL) == -1){
				
				printErrorMessage(EXEC_ERROR);
				return EXIT;
			}
		}
		else{																// Commands for the parent process
			
			int j, fd_write, fd_read;
				
			fd_write = open(pathname_write, O_WRONLY);
			if(fd_write < 0){
				printErrorMessage(OPEN_ERROR);
				return EXIT;
			}

			fd_read = open(pathname_read, O_RDONLY);
			if(fd_read < 0){
				printErrorMessage(OPEN_ERROR);
				return EXIT;
			}

			for(j = 0; j < distr[i]; j++, current_map_position++){
				
				int k = current_map_position;
				
				char* string;
				string = (char*)malloc((int)strlen(mapPtr[k].dirPath)+1);
				if(string == NULL){
					printErrorMessage(MEM_ERROR);
					return EXIT;
				}

				strcpy(string,mapPtr[k].dirPath);


				char length[1024];											
				memset(length,'\0',1024);
				sprintf(length,"%ld",strlen(string));
				// printf("(Parent) Sending %s\n",length);
				write(fd_write,length,1024);								// Send the length of the path as a string
				
				char response[3];
				read(fd_read,response,strlen("OK"));
				response[2] = '\0';
				// printf("(Parent) %s\n",response);
				if(strcmp(response,"OK") == 0)
					write(fd_write,string,strlen(string));

				free(string);
				read(fd_read,response,strlen("OK"));
				response[2] = '\0';
				if(strcmp(response,"OK") != 0)
					return EXIT;
			}
									
			close(fd_write);
			close(fd_read);
		}

		free(pathname_read);
		free(pathname_write);
	}

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***************************/

	deletePipeArray(&pipes_ptr,numWorkers);
	deleteMap(&mapPtr,total_directories);
}