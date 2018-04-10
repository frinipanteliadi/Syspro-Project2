#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
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

	fclose(fp);																// Closing the file

	// printMap(mapPtr,total_directories);
	

	/****************************/
	/*** CREATING THE WORKERS ***/
	/***         AND          ***/
	/***      THE PIPES       ***/
	/****************************/
	
	char buffer[sizeof(int)*4+1];
	char id[sizeof(int)*4+1];
	char* pathname_read;
	char* pathname_write;
	// int reading_fd[numWorkers];												
	int writing_fd[numWorkers];
	int fd;
	pid_t pid;
	
	int distributions = total_directories%numWorkers;
	int counter = 0;
	int total = 0;															// Keeps track of the times we've written to the file so far

	for(int i = 0; i < numWorkers; i++){
		sprintf(buffer,"%d",i);
		
		errorCode = allocatePathname(&pathname_read,&pathname_write,
			strlen(buffer));
		if(errorCode != OK){
			printErrorMessage(errorCode);
			return EXIT;
		}

		createPathname(&pathname_read,buffer,&pathname_write);				
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
			if(execlp("./worker","./worker","testing.txt"/*pathname_read,
				pathname_write*/,(char*)NULL) == -1){
				printErrorMessage(EXEC_ERROR);
				return EXIT;
			}
			continue;
		}
		else{
			// wait(NULL);
			while(counter < total_directories){
				int j;
				for(j = 0; j < distributions; j++)
					if((counter+j) >= total_directories)
						break;

				int turn = 0;
				for(int k = counter; turn < j; k++){
					sprintf(id,"%d",mapPtr[k].dirID);
					char* string;
					string = (char*)malloc(+sizeof(id)+strlen(" ")
						+strlen(mapPtr[k].dirPath)+1);
					if(string == NULL){
						printErrorMessage(MEM_ERROR);
						return EXIT;
					}
					strcpy(string,id);
					strcat(string," ");
					strcat(string,mapPtr[k].dirPath);

					// printf("String: %s\n",string);

					int bytes;
					if(total == 0){											// It's the first time we're writing to the file, therefore we must create it
						mode_t fdmode = S_IRUSR|S_IWUSR;
						fd = open(/*pathname_write*/"testing.txt", 
							O_WRONLY | O_CREAT, fdmode);
						bytes = write(fd,string,strlen(string));
					}
					else{
						fd = open(/*pathname_write*/ "testing.txt", O_WRONLY | O_APPEND);
						bytes = write(fd,string,strlen(string));
					}

					total++;					
					close(fd);
					free(string); 					
					turn++;
				}
				counter += j;
			}
			

			// writing_fd[i] = open(pathname_write,O_WRONLY);
			// write(writing_fd[i],"Hello",strlen("Hello")+1);
			/*printf("(Parent)Reads from: %s\n",pathname_read);
			printf("(Parent)Writes to: %s\n",pathname_write);*/
		}

		free(pathname_read);
		free(pathname_write);
	}

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***************************/

	deleteMap(&mapPtr,total_directories);
}