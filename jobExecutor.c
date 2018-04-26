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
	
	/* Storing all information on pipes */
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

			errorCode = addToPipeArray(&pipes_ptr,i,
				pathname_read,pathname_write,fd_read,fd_write);
			if(errorCode != OK){
				printErrorMessage(errorCode);
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

				errorCode = writingPipes(string,fd_write,fd_read);
				if(errorCode != OK)
					return EXIT;

				free(string);
			}
									
			// close(fd_write);
			// close(fd_read);
		}

		free(pathname_read);
		free(pathname_write);
	}

	/*********************************/
	/*** USER REQUESTED OPERATIONS ***/
	/*********************************/

	welcomeMessage();
	printf("\n");

	size_t n = 0;
	char* input = NULL;
	while(getline(&input,&n,stdin)!=-1){
		
		/* Remove the newline character from the input */
		input = strtok(input,"\n");					
		
		/* E.g /search quick brown lazy */
		char* input_copy;
		input_copy = (char*)malloc((strlen(input)+1)*sizeof(char));
		if(input_copy == NULL)
			return MEM_ERROR;
		strcpy(input_copy,input);

		char* operation = strtok(input," \t");	
		/* operation = "/search" & input = "/search" */

		char* arguments = strtok(NULL,"\n");		
		/* arguments = "quick brown lazy" */

		if(strcmp(operation,"/search") == 0 ||
			strcmp(operation,"/maxcount") == 0 ||
			strcmp(operation,"/mincount") == 0 ||
			strcmp(operation,"/wc") == 0){
			printf("Requested an operation\n");


			for(int i = 0; i < numWorkers; i++){

				/* SENDING THE OPERATION */
				
				/* Send the size to the other side */
				char length[1024];
				memset(length,'\0',1024);
				sprintf(length,"%ld",strlen(input_copy));
				write(pipes_ptr[i].pipe_write_fd,length,1024);

				/* Wait for a positive response */
				char response[3];
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';

				if(strcmp(response,"OK") != 0)
					return EXIT;

				/* Send the data */
				write(pipes_ptr[i].pipe_write_fd,input_copy,strlen(input_copy));
				
				/* Read the response */
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';
				if(strcmp(response,"OK") != 0)
					return EXIT;

				/* GETTING THE RESULTS */
				
				/* Find out the number of results to expect */
				int results;
				read(pipes_ptr[i].pipe_read_fd,&results,sizeof(int));

				/* Respond positively */
				write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));	
				
				printf("The parent will receive %d results from Worker[%d]\n",results,i);
			
				for(int j = 0; j < results; j++){

					char length[1024];
					memset(length,'\0',1024);
					read(pipes_ptr[i].pipe_read_fd,length,1024);
					int read_length = atoi(length);

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

					/* Get the word */
					char* word;
					word = (char*)malloc((read_length+1)*sizeof(char));
					if(word == NULL)
						return MEM_ERROR;

					/* Get the path of the file */
					read(pipes_ptr[i].pipe_read_fd,word,(size_t)read_length*sizeof(char));
					word[read_length] = '\0';

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));





					/* Get the length of the file's path */
					memset(length,'\0',1024);
					read(pipes_ptr[i].pipe_read_fd,length,1024);
					length[strlen(length)] = '\0';
					read_length = atoi(length);

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

					char* file_path;
					file_path = (char*)malloc((read_length+1)*sizeof(char));
					if(file_path == NULL)
						return MEM_ERROR;

					/* Get the path of the file */
					read(pipes_ptr[i].pipe_read_fd,file_path,(size_t)read_length*sizeof(char));
					file_path[read_length] = '\0';

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));


					/* Get the index of the line */
					int line_index;
					read(pipes_ptr[i].pipe_read_fd,&line_index,sizeof(int));

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));




					/* Get the content of the line */	
					memset(length,'\0',1024);
					read(pipes_ptr[i].pipe_read_fd,length,1024);
					length[strlen(length)] = '\0';
					read_length = atoi(length);

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

					char* line_content;
					line_content = (char*)malloc((read_length+1)*sizeof(char));
					if(line_content == NULL)
						return MEM_ERROR;

					/* Get the content of the line */
					read(pipes_ptr[i].pipe_read_fd,line_content,(size_t)read_length*sizeof(char));
					line_content[read_length] = '\0';

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));


					printf("Result[%d] - %s:\n",j,word);
					printf(" *FilePath: %s\n",file_path );
					printf(" *Line(%d): %s\n\n",line_index,line_content);
					
					free(word);
					free(file_path);
					free(line_content);
				}

			}


			free(input_copy);
		}
		else if(strcmp(operation,"/exit") == 0){
			printf("Exiting the application\n");

			for(int i = 0; i < numWorkers; i++){

				/* Send the size to the other side */
				char length[1024];
				memset(length,'\0',1024);
				sprintf(length,"%ld",strlen("/exit"));
				write(pipes_ptr[i].pipe_write_fd,length,1024);

				/* Wait for a positive response */
				char response[3];
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';

				if(strcmp(response,"OK") != 0)
					return EXIT;

				/* Send the data */
				write(pipes_ptr[i].pipe_write_fd,"/exit",strlen("/exit"));
				
				/* Read the response */
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';
				if(strcmp(response,"OK") != 0)
					return EXIT;
			}
			
			free(input_copy);
			break;
		}
		else{
			printf("Invalid input. Try again\n");
			free(input_copy);
		}
		printf("\n\n");
	}

	printf("\n");
	free(input);

	// printPipeArray(&pipes_ptr,numWorkers);

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***        AND          ***/
	/***  CLOSING THE PIPES  ***/
	/***************************/

	closingPipes(&pipes_ptr,numWorkers);
	deletePipeArray(&pipes_ptr,numWorkers);
	deleteMap(&mapPtr,total_directories);
}