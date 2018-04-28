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

	/* Find the actual number of workers we are going to create */
	int total_empty = 0;
	setDistributions(total_directories,numWorkers,distr);
	for(int i = 0; i < numWorkers; i++){
		if(distr[i] == 0)
			total_empty++;
		// printf("Worker%d: %d directories\n",i,distr[i]);
	}

	if(total_empty != 0)
		numWorkers -= total_empty;

	printf("We are going to create %d workers instead, because",numWorkers);
	printf(" we cannot distribute the directories evenly\n");

	/* Allocating memory for the result_map structure */
	result_map* resultPtr;
	resultPtr = (result_map*)malloc(numWorkers*sizeof(result_map));
	if(resultPtr == NULL)
		return MEM_ERROR;
	
	/* Initializing the result_map structure with NULL and zero values */
	for(int i = 0; i < numWorkers; i++){
		resultPtr[i].ptr = NULL;
		resultPtr[i].total_results = 0;
	}


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
			
		/* The user requested to exit the app */
		if(strcmp(input,"/exit") == 0){
			printf("Exiting the application\n");

			/* Inform each one of the children */
			for(int i = 0; i < numWorkers; i++){
				
				/* Send the length of the /exit string */
				char length[1024];
				memset(length,'\0',1024);
				sprintf(length,"%ld",strlen("/exit"));
				write(pipes_ptr[i].pipe_write_fd,length,1024);

				char response[3];
				memset(response,'\0',3);
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';

				if(strcmp(response,"OK") != 0)
					return EXIT;
				else
					write(pipes_ptr[i].pipe_write_fd,"/exit",strlen("/exit"));

				memset(response,'\0',3);
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';
				if(strcmp(response,"OK") != 0)
					return EXIT;

			}

			break;
		}

		char* input_copy;
		input_copy = (char*)malloc((strlen(input)+1)*sizeof(char));
		if(input_copy == NULL)
			return MEM_ERROR;
		strcpy(input_copy,input);

		char* operation;
		char* arguments;

		operation = strtok(input_copy," \t");
		arguments = strtok(NULL, "\n");

		printf("Operation: %s\n",operation);
		printf("Arguments: %s\n",arguments);

		if(strcmp("/search",operation) == 0){

			/* Find the total number of words in the input */
			int total_args = getNumberOfArgs(input);

			/* Store each one of the arguments and its length */
			char** wordKeeping;
			wordKeeping = (char**)malloc(total_args*sizeof(char*));
			if(wordKeeping == NULL)
				return MEM_ERROR;

			int individual_lengths[total_args];

			errorCode = storingWords(wordKeeping,input,individual_lengths);
			if(errorCode != OK)
				return EXIT;

			/* Getting the time limit for the workers */
			int deadline;
			if(strcmp(wordKeeping[total_args-2],"-d") == 0)
				deadline = atoi(wordKeeping[total_args-1]);
			else{
				printf("The deadline was not specified.\n");
				printf("Try again\n");

				for(int i = 0; i < total_args; i++)
					free(wordKeeping[i]);
				free(wordKeeping);
				free(input_copy);
				continue;
			}

			// printf("The deadline is :%d\n",deadline);
			
			/* The total length of the combined arguments */
			int total_length = 0;
			for(int i = 0; i < total_args-2; i++)
				total_length += individual_lengths[i];

			/* The size of the first message */
			int size = total_length + (total_args-2);

			/* The message that will be sent to the workers */
			char* message;
			message = (char*)malloc(size*sizeof(char));
			if(message == NULL)
				return MEM_ERROR;
			memset(message,'\0',size);

			/* Creating the message */
			for(int i = 0; i < total_args-2; i++){

				strcat(message,wordKeeping[i]);
				if(i < total_args-3)
					strcat(message," ");
			}

			/* Send the operation to each one of the workers */
			for(int i = 0; i < numWorkers; i++){

				/* Sending the length of the message */
				char length[1024];
				memset(length,'\0',1024);
				sprintf(length,"%ld",strlen(message));
				write(pipes_ptr[i].pipe_write_fd,length,1024);

				/* Get the child's response */
				char response[3];
				memset(response,'\0',3);
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';

				if(strcmp(response,"OK") != 0)
					return EXIT;

				/* Send the message */
				write(pipes_ptr[i].pipe_write_fd,message,strlen(message));

				/* Get the child's response */
				memset(response,'\0',3);
				read(pipes_ptr[i].pipe_read_fd,response,strlen("OK"));
				response[2] = '\0';
				if(strcmp(response,"OK") != 0)
					return EXIT;
				
				/* Find out the total number of incoming results */
				int results;
				read(pipes_ptr[i].pipe_read_fd,&results,sizeof(int));

				write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

				printf("The parent found out that %d results will come\n",results);

				/* Allocating memory for the current worker's result_info structure */
				resultPtr[i].ptr = (result_info*)malloc(results*sizeof(result_info));
				if(resultPtr[i].ptr == NULL)
					return MEM_ERROR;
					
				/* Initializing the result_info structure with NULL and zero values */
				for(int j = 0; j < results; j++){
					resultPtr[i].total_results = results;
					resultPtr[i].ptr[j].word = NULL;
					resultPtr[i].ptr[j].file_path = NULL;
					resultPtr[i].ptr[j].line_number = 0;
					resultPtr[i].ptr[j].line_content = NULL;
				}

				/* Start retieving the results */
				for(int j = 0; j < results; j++){

					/* 1) Get the word */
					char length[1024];
					memset(length,'\0',1024);
					read(pipes_ptr[i].pipe_read_fd,length,1024);
					int read_length = atoi(length);

					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

					resultPtr[i].ptr[j].word = (char*)malloc((read_length+1)*sizeof(char));
					if(resultPtr[i].ptr[j].word == NULL)
						return MEM_ERROR;

					read(pipes_ptr[i].pipe_read_fd,resultPtr[i].ptr[j].word,(size_t)read_length*sizeof(char));
					resultPtr[i].ptr[j].word[read_length] = '\0';

					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));
					

					/* 2) Get the file of the path */

					/* Get the length of the file's path */
					memset(length,'\0',1024);
					read(pipes_ptr[i].pipe_read_fd,length,1024);
					length[strlen(length)] = '\0';
					read_length = atoi(length);

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

					resultPtr[i].ptr[j].file_path = (char*)malloc((read_length+1)*sizeof(char));
					if(resultPtr[i].ptr[j].file_path == NULL)
						return MEM_ERROR;

					/* Get the path of the file */
					read(pipes_ptr[i].pipe_read_fd,resultPtr[i].ptr[j].file_path,(size_t)read_length*sizeof(char));
					resultPtr[i].ptr[j].file_path[read_length] = '\0';

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));



					/* 3) Get the index of the line */

					read(pipes_ptr[i].pipe_read_fd,&resultPtr[i].ptr[j].line_number,sizeof(int));
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));	


					/* 4) Get the line */
					memset(length,'\0',1024);
					read(pipes_ptr[i].pipe_read_fd,length,1024);
					length[strlen(length)] = '\0';
					read_length = atoi(length);

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));

					resultPtr[i].ptr[j].line_content = (char*)malloc((read_length+1)*sizeof(char));
					if(resultPtr[i].ptr[j].line_content == NULL)
						return MEM_ERROR;

					/* Get the content of the line */
					read(pipes_ptr[i].pipe_read_fd,resultPtr[i].ptr[j].line_content,(size_t)read_length*sizeof(char));
					resultPtr[i].ptr[j].line_content[read_length] = '\0';

					/* Respond positively */
					write(pipes_ptr[i].pipe_write_fd,"OK",strlen("OK"));			
				}
			}

			/* Different results arrive in each iteration.
			   Therefore, we must release all of the memory 
			   we previously allocated, in order to prepare
			   for the next results */
			for(int j = 0; j < numWorkers; j++){
				for(int k = 0; k < resultPtr[j].total_results; k++){
					printf("\n\n");
					printf("*resultPtr[%d].ptr[%d].word: '%s'\n",j,k,resultPtr[j].ptr[k].word);
					printf("*resultPtr[%d].ptr[%d].file_path: %s\n",j,k,resultPtr[j].ptr[k].file_path);
					printf("*resultPtr[%d].ptr[%d].line_number: %d\n",j,k,resultPtr[j].ptr[k].line_number);
					printf("*resultPtr[%d].ptr[%d].line_content: %s\n",j,k,resultPtr[j].ptr[k].line_content);
					printf("\n\n");

					free(resultPtr[j].ptr[k].word);
					free(resultPtr[j].ptr[k].file_path);
					free(resultPtr[j].ptr[k].line_content);
				}
				free(resultPtr[j].ptr);
				resultPtr[j].total_results = 0;
				resultPtr[j].ptr = NULL;
			}

			/* Freeing the memory we previously allocated */
			for(int i = 0; i < total_args; i++){
				// printf("wordKeeping[%d]: %s\n",i,wordKeeping[i]);
				free(wordKeeping[i]);
			}

			free(wordKeeping);
			free(message);
		}
		else
			printf("Invalid input. Try again\n");

		free(input_copy);
	}

	printf("\n");
	free(input);

	// printPipeArray(&pipes_ptr,numWorkers);

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***        AND          ***/
	/***  CLOSING THE PIPES  ***/
	/***************************/
	// for(int i = 0; i < numWorkers; i++){
	// 	for(int j = 0; j < resultPtr[i].total_results; j++){
	// 		printf("\nresultPtr[%d].ptr[%d].word: %s\n",i,j,resultPtr[i].ptr[j].word);
	// 		printf("resultPtr[%d].ptr[%d].file_path: %s\n",i,j,resultPtr[i].ptr[j].file_path);
	// 		printf("resultPtr[%d].ptr[%d].line_content: %s\n\n",i,j,resultPtr[i].ptr[j].line_content);
	// 		free(resultPtr[i].ptr[j].word);
	// 		free(resultPtr[i].ptr[j].file_path);
	// 		free(resultPtr[i].ptr[j].line_content);
	// 	}
	// 	free(resultPtr[i].ptr);
	// }
	free(resultPtr);
	closingPipes(&pipes_ptr,numWorkers);
	deletePipeArray(&pipes_ptr,numWorkers);
	deleteMap(&mapPtr,total_directories);
}