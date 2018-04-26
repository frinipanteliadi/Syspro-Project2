#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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
		case OPEN_ERROR:
			printf("Failed to open the file (named-pipe)\n");
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

void distributions(int dirs, int workers, int* array){
	int a = dirs;
	int b = workers;

	for(int i = 0; i < workers; i++){
		int result = a%b;

		if(result != 0)
			array[i] = result;
		else{
			if(b == 1)
				array[i] = a;
			else if(a == b){
				for(int j = i; j < workers; j++)
					array[j] = 1;
				return;
			}
			else{
				for(int j = i; j < workers; j++)
					array[j] = a/b;
				return;
			}
		}

		a -= result;
		b--;
	}
}


void setDistributions(int dirs, int workers, int* array){
	if(dirs < workers){
		for(int i = dirs; i < workers; i++)
			array[i] = 0;
		distributions(dirs, dirs,array);
	}
	else
		distributions(dirs,workers,array);
}

void welcomeMessage(void){
	printf("\nWelcome! Please, choose one of the following");
	printf(" options to continue.\n");
	printf("* /search\n");
	printf("* /maxcount (keyword)\n");
	printf("* /mincount (keyword)\n");
	printf("* /wc\n");
	printf("* /exit\n");
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

/* Allocates memory in the heap for the pathname of 
   of the named pipe */
int allocatePathname(char** ptr_read, char** ptr_write, int length){
	*ptr_read = (char*)malloc(strlen("PipeRead")+length+1);
	if(*ptr_read == NULL)
		return MEM_ERROR;

	*ptr_write = (char*)malloc(strlen("PipeWrite")+length+1);
	if(*ptr_write == NULL)
		return MEM_ERROR;

	return OK;
}

/* Creates the pathname of the named pipe */
void createPathname(char** ptr_read, char* buffer,
	char** ptr_write){
	
	strcpy(*ptr_read,"PipeRead");
	strcat(*ptr_read,buffer);

	strcpy(*ptr_write,"PipeWrite");
	strcat(*ptr_write,buffer);
}

/* Allocates memory for the creation of an array
   in which we will store the names of the pipes */
int allocatePipeArray(pipes** ptr, int size){
	*ptr = (pipes*)malloc(size*sizeof(pipes));
	if(*ptr == NULL)
		return MEM_ERROR;
	return OK;
}

int addToPipeArray(pipes** ptr,int index, char* read, char* write,
 int read_fd, int write_fd){
	ptr[0][index].pipe_id = index;
	
	ptr[0][index].pipe_read_fd = read_fd;

	ptr[0][index].pipe_write_fd = write_fd;

	ptr[0][index].pipename_read = (char*)malloc(strlen(read)+1);
	if(ptr[0][index].pipename_read == NULL)
		return MEM_ERROR;
	strcpy(ptr[0][index].pipename_read,read);

	ptr[0][index].pipename_write = (char*)malloc(strlen(write)+1);
	if(ptr[0][index].pipename_write == NULL)
		return MEM_ERROR;
	strcpy(ptr[0][index].pipename_write,write);

	return OK;
}

void printPipeArray(pipes** ptr, int size){
	for(int i = 0; i < size; i++){
		printf("*******************\n");
		printf("* PipeID: %d\n",ptr[0][i].pipe_id);
		printf("* Reading Pipe Fd: %d\n",ptr[0][i].pipe_read_fd);
		printf("* Writing Pipe Fd: %d\n",ptr[0][i].pipe_write_fd);
		printf("* Pipe for Reading: %s\n",ptr[0][i].pipename_read);
		printf("* Pipe for Writing: %s\n",ptr[0][i].pipename_write);
		printf("*******************\n\n");
	}
}

void deletePipeArray(pipes** ptr, int size){
	for(int i = 0; i < size; i++){
		free(ptr[0][i].pipename_read);
		free(ptr[0][i].pipename_write);
	}

	free(*ptr);
}

void closingPipes(pipes** ptr, int size){
	for(int i = 0; i < size; i++){
		close(ptr[0][i].pipe_read_fd);
		close(ptr[0][i].pipe_write_fd);
	}
}

/* Actions for the process which does the writing */
int writingPipes(char* data, int fd_write, int fd_read){
	
	/* Write the size of the data in the pipe */
	char length[1024];
	memset(length,'\0',1024);
	sprintf(length,"%ld",strlen(data));
	write(fd_write,length,1024);

	/* Check that everything went smoothly */
	char response[3];
	read(fd_read,response,strlen("OK"));
	response[2] = '\0';

	/* Write the data in the pipe */
	if(strcmp(response,"OK") != 0)
		return EXIT;
	else
		write(fd_write,data,strlen(data));

	read(fd_read,response,strlen("OK"));
	response[2] = '\0';
	if(strcmp(response,"OK") != 0)
		return EXIT;

	return OK;
}

/* Actions for the process which does the reading */
int readingPipes(char** data, int fd_read, int fd_write){
	
	/* Retrieve the size of the data about to be read */
	char length_array[1024];
	int length;
	read(fd_read,length_array,1024);
	length_array[strlen(length_array)] = '\0';
	length = atoi(length_array);

	/* Inform the other side that everything went according to plan */
	write(fd_write,"OK",strlen("OK"));

	/* Allocate memory for the data which is about to be read */
	*data = (char*)malloc((length+1)*sizeof(char));
	if(*data == NULL)
		return MEM_ERROR;

	/* Read the data from the pipe */
	read(fd_read,*data,(size_t)length*sizeof(char));
	data[0][length] = '\0';

	write(fd_write,"OK",strlen("OK"));
	return OK;
}
