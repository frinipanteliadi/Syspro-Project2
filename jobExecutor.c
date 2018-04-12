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
	char id[sizeof(int)*4+1];
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

		/*printf("\nPathnameRead: %s\n",pathname_read);
		printf("PathnameWrite: %s\n",pathname_write);*/

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
		else{																// Commands for the parent process
			/*int fd = open(pathname_write,O_WRONLY | O_APPEND);
			if(fd < 0){
				printErrorMessage(OPEN_ERROR);
				return EXIT;
			}*/
		
			int j, fd, turn = 0;
			ssize_t bytes = 0;
			for(j = 0; j < distr[i]; j++){
				
				int k = current_map_position;
				
				sprintf(id,"%d",mapPtr[k].dirID);
				char* string;
				string = (char*)malloc((int)strlen(id)+(int)strlen(" ")
					+(int)strlen(mapPtr[k].dirPath)+1);
				if(string == NULL){
					printErrorMessage(MEM_ERROR);
					return EXIT;
				}

				strcpy(string,id);
				strcat(string," ");
				strcat(string,mapPtr[k].dirPath);
				
				// printf("%d) %s\n",j,string);
				if(k == 0)
					fd = open(pathname_write, O_WRONLY | O_CREAT);
				else
					fd = open(pathname_write, O_WRONLY | O_APPEND);

				if(fd < 0){
					printErrorMessage(OPEN_ERROR);
					return EXIT;
				}
				
				// printf("Writing %s\n",string);
				bytes += write(fd,string,(size_t)(strlen(string)));
				
				/*close(fd);*/
				free(string);
				current_map_position++;	
			}
			
			// printf("The parent wrote %d bytes to the file\n",bytes);	
			
			int filedesc = open("info", O_CREAT | O_RDWR, 0666);
			if(filedesc < 0){
				printf("Attempt to create an extra file");
				printf(" was unsuccessful\n");
				return EXIT;
			}

			write(filedesc,&bytes,sizeof(ssize_t));
			close(filedesc);

			kill(pid,SIGCONT);												// Time for the child to read the file
			raise(SIGSTOP);													// Parent will wait for the child

			close(fd);
			/*printf("(Parent)Reads from: %s\n",pathname_read);
			printf("(Parent)Writes to: %s\n",pathname_write);*/
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