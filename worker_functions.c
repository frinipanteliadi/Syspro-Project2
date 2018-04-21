#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
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
		case WORKER_DIR_OPEN_ERROR:
			printf("Failed to open a directory\n");
			break;
		case WORKER_DIR_CLOSE_ERROR:
			printf("Failed to close a directory\n");
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

/* Initializes the i-th member of the Map */
int initializeWorkerMap(worker_map** map_ptr, int i, int fd, int string_length){
	
	if(asprintf(&map_ptr[0][i].dirID,"%d",i) < 0)
		return WORKER_MEM_ERROR;

	// ptr[0][i].dirID = i;

	map_ptr[0][i].dirPath = (char*)malloc((string_length+1)*sizeof(char));
	if(map_ptr[0][i].dirPath == NULL)
		return WORKER_MEM_ERROR;

	read(fd,map_ptr[0][i].dirPath,(size_t)string_length*sizeof(char));
	map_ptr[0][i].dirPath[string_length] = '\0';

	map_ptr[0][i].total_files = 0;
	map_ptr[0][i].dirFiles = NULL;

	return WORKER_OK;
}

/* Prints the map (Helpful for debugging) */
void printWorkerMap(worker_map** ptr, int size){
	for(int i = 0; i < size; i++){
		printf("**************************\n");
		printf("*dirID: %s\n",ptr[0][i].dirID);
		printf("*dirPath: %s",ptr[0][i].dirPath);
		// printf("*dirFiles: %d\n",ptr[0][i].dirFiles);
		printf("*Total Files: %d\n",ptr[0][i].total_files);
		printf("**************************\n");
	}
}

/* Deletes all memory associated with the map */
void deleteWorkerMap(worker_map** ptr, int size){
	for(int i = 0; i < size; i++){
		free(ptr[0][i].dirID);
		free(ptr[0][i].dirPath);
	}
	free(*ptr);
}


/**********************/
/*** PIPE FUNCTIONS ***/
/**********************/

/* Opens the pipes for reading and writing */
int openingPipes(char* reading_pipe, char* writing_pipe, 
	int* fd_read, int* fd_write){

	*fd_read = open(reading_pipe, O_RDONLY);
	if(*fd_read < 0)
		return WORKER_OPEN_ERROR;

	*fd_write = open(writing_pipe, O_WRONLY);
	if(*fd_write < 0)
		return WORKER_OPEN_ERROR;

	return WORKER_OK;
}

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

int fileInformation(int distr, worker_map** map_ptr){
	
	for(int i = 0; i < distr; i++){

		/* Creating the full path of the directory
		   for scandir() */
		char* name;
		name = (char*)malloc((strlen(map_ptr[0][i].dirPath)+1)*sizeof(char));
		if(name == NULL)
			return WORKER_MEM_ERROR;
		strncpy(name,map_ptr[0][i].dirPath,strlen(map_ptr[0][i].dirPath)-1);
		name[strlen(map_ptr[0][i].dirPath)-1] = '/';
		name[strlen(map_ptr[0][i].dirPath)] = '\0';

		/* Scan the directory */
		struct dirent **entry;
		int n = scandir(name,&entry,NULL,alphasort);
		if(n < 0){
			printf("Failed to scan the directory: %s\n",name);
			return -1;
		}
		
		/* Remove both the current and the parent directory */
		int actual_files = n-2;
		map_ptr[0][i].total_files = actual_files;

		/* Allocate memory for the structure */
		map_ptr[0][i].dirFiles = (file_info*)malloc(map_ptr[0][i].total_files*sizeof(file_info));
		if(map_ptr[0][i].dirFiles == NULL)
			return WORKER_MEM_ERROR;

		file_info* temp_ptr;
		temp_ptr = map_ptr[0][i].dirFiles;

		int file_no = 0;
		/* Add the name of each file and its full path */
		for(int k = 2, j = 0; k < n; k++, j++, file_no++){
				
			char* temp_id;
			if(asprintf(&temp_id,"%d",file_no) < 0)
				return WORKER_MEM_ERROR;

			temp_ptr[j].file_id = (char*)malloc((strlen(map_ptr[0][i].dirID)+strlen(temp_id)+2)*sizeof(char));
			if(temp_ptr[j].file_id == NULL)
				return WORKER_MEM_ERROR;
			strcpy(temp_ptr[j].file_id,map_ptr[0][i].dirID);
			strcat(temp_ptr[j].file_id,"|");
			strcat(temp_ptr[j].file_id,temp_id);

			free(temp_id);

			temp_ptr[j].file_name = (char*)malloc((strlen(entry[k]->d_name)+1)*sizeof(char));
			if(temp_ptr[j].file_name == NULL)
				return WORKER_MEM_ERROR;
			strncpy(temp_ptr[j].file_name,entry[k]->d_name,strlen(entry[k]->d_name));
			temp_ptr[j].file_name[strlen(entry[k]->d_name)] = '\0';

			temp_ptr[j].full_path = (char*)malloc((strlen(name)+strlen(entry[k]->d_name)+1)*sizeof(char));
			if(temp_ptr[j].full_path == NULL)
				return WORKER_MEM_ERROR;
			strcpy(temp_ptr[j].full_path,name);
			strcat(temp_ptr[j].full_path,entry[k]->d_name);
		}

		/* Release the memory that scandir() allocated */
		for(int j = 0; j < n; j++)
			free(entry[j]);
		free(entry);
	
		free(name);
	}
	return WORKER_OK;
}

int setLines(int distr, worker_map** map_ptr){

	for(int i = 0; i < distr; i++){

		for(int j = 0; j < map_ptr[0][i].total_files; j++){

			file_info* temp_ptr;
			temp_ptr = map_ptr[0][i].dirFiles;

			FILE* fp;
			fp = fopen(temp_ptr[j].full_path,"r");
			if(fp == NULL){
				printf("Failed to open the file: %s\n",temp_ptr[j].file_name);
				return -1;
			}

			temp_ptr[j].lines = getNumberOfLines(fp);

			rewind(fp);
			if(fclose(fp) != 0){
				printf("Failed to close the file: %s\n",temp_ptr[j].file_name);
				return -1;
			}
		}
	}
	return WORKER_OK;
}

int readLines(int distr, worker_map** map_ptr){

	for(int i = 0; i < distr; i++){
		for(int j = 0; j < map_ptr[0][i].total_files; j++){

			map_ptr[0][i].dirFiles[j].ptr = (line_info*)malloc(map_ptr[0][i].dirFiles[j].lines*sizeof(line_info));
			if(map_ptr[0][i].dirFiles[j].ptr == NULL)
				return WORKER_MEM_ERROR;
		
			FILE* fp;
			fp = fopen(map_ptr[0][i].dirFiles[j].full_path,"r");
			if(fp == NULL){
				printf("Failed to open the file: %s\n",map_ptr[0][i].dirFiles[j].file_name);
				return -1;
			}

			int k = 0;
			size_t n = 0;
			char* line = NULL;
			while(getline(&line,&n,fp) != -1){

				map_ptr[0][i].dirFiles[j].ptr[k].line_content = (char*)malloc((strlen(line)+1)*sizeof(char));
				if(map_ptr[0][i].dirFiles[j].ptr[k].line_content == NULL)
					return WORKER_MEM_ERROR;
				strcpy(map_ptr[0][i].dirFiles[j].ptr[k].line_content,line);
				
				char* temp_id;
				if(asprintf(&temp_id,"%d",k) < 0)
					return WORKER_MEM_ERROR;

				map_ptr[0][i].dirFiles[j].ptr[k].id = (char*)malloc((strlen(map_ptr[0][i].dirFiles[j].file_id)+strlen(temp_id)+2)*sizeof(char));
				if(map_ptr[0][i].dirFiles[j].ptr[k].id == NULL)
					return WORKER_MEM_ERROR;
				strcpy(map_ptr[0][i].dirFiles[j].ptr[k].id, map_ptr[0][i].dirFiles[j].file_id);
				strcat(map_ptr[0][i].dirFiles[j].ptr[k].id, "|");
				strcat(map_ptr[0][i].dirFiles[j].ptr[k].id,temp_id);

				// map_ptr[0][i].dirFiles[j].ptr[k].id = k;
				free(temp_id);
				k++;
			}

			free(line);
			rewind(fp);

			if(fclose(fp) != 0){
				printf("Failed to close the file: %s\n",map_ptr[0][i].dirFiles[j].file_name);
				return -1;
			}
		}
	}

	return WORKER_OK;
}

int initializeStructs(int distr, worker_map** map_ptr){

	int errorCode;
	
	errorCode = fileInformation(distr,map_ptr);
	if(errorCode != WORKER_OK)
		return errorCode;

	errorCode = setLines(distr,map_ptr);
	if(errorCode != WORKER_OK)
		return errorCode;

	errorCode = readLines(distr,map_ptr);
	if(errorCode != WORKER_OK)
		return errorCode;

	return WORKER_OK;
}

void printDirectory(int distr, worker_map** map_ptr){
	for(int i = 0; i < distr; i++){
		printf("\n\n");
		printf("*Directory: %s",map_ptr[0][i].dirPath);
		for(int j = 0; j < map_ptr[0][i].total_files; j++){
			printf("\n");
			printf(" -Name: %s\n",map_ptr[0][i].dirFiles[j].file_name);
			printf(" -ID: %s\n",map_ptr[0][i].dirFiles[j].file_id);
			printf(" -Path: %s\n",map_ptr[0][i].dirFiles[j].full_path);
			printf(" -Lines: %d\n",map_ptr[0][i].dirFiles[j].lines);

			for(int k = 0; k < map_ptr[0][i].dirFiles[j].lines; k++)
				printf("Line %s) %s\n",map_ptr[0][i].dirFiles[j].ptr[k].id,map_ptr[0][i].dirFiles[j].ptr[k].line_content);		
		}
	}
}


/* Creates and initializes the root of the Trie */
int createRoot(trieNode **root){

	*root = (trieNode*)malloc(sizeof(trieNode));
	if(*root == NULL)
		return WORKER_MEM_ERROR;

	(**root).letter = 0;
	(**root).isEndOfWord = False;
	(**root).children = NULL;
	(**root).next = NULL;

	return WORKER_OK;
}

/* Returns the difference between the ASCII values
   of a and b */
int compareKeys(char* a, char* b){
	return((*a)-(*b));
} 

/* Inserts a new value (word) in the Trie */
int insertTrie(trieNode* node, char* word){
	trieNode* temp = node; 
	trieNode* parent = node;
	trieNode* previous = NULL;
	int value; 
	int errorCode;

	// printf("Inserting the word: %s\n",word);

	/* Each iteration is responsible for a single
	   letter of the word we're trying to insert */
	for(int i = 0; i < (int)strlen(word); i++){

		if(temp->children == NULL){
			temp->children = (trieNode*)malloc(sizeof(trieNode));
			if(temp->children == NULL)
				return WORKER_MEM_ERROR;

			temp->children->letter = word[i];
			temp->children->isEndOfWord = False;
			temp->children->children = NULL;
			temp->children->next = NULL;

			temp = temp->children;

			if(i == strlen(word)-1) {
				temp->isEndOfWord = True;
			}
		}

		else{
			parent = temp;
			temp = temp->children;

			while(temp != NULL){
				value = compareKeys(&word[i],&(temp->letter));

				if(value > 0){
					previous = temp;
					temp = temp->next;
					continue;
				}

				if(value <= 0)
					break;
			}

			if(value > 0){
				temp = (trieNode*)malloc(sizeof(trieNode));
				if(temp == NULL)
					return WORKER_MEM_ERROR;
				temp->next = previous->next;
				previous->next = temp;
				temp->letter = word[i];
				temp->isEndOfWord = False;
				temp->children = NULL;
			}

			if(value < 0){
				/* Up until the previous iteration, the letters we
				   were trying to insert were already in the Trie. 
				   This is the first letter that differs and we have 
				   to insert it as the last element of the list */

				/* Insert at the beginning */
				if(previous == NULL){
					previous = (trieNode*)malloc(sizeof(trieNode));
					if(previous == NULL)
						return WORKER_MEM_ERROR;
					previous->letter = word[i];

					previous->isEndOfWord = False;
					previous->children = NULL;
					previous->next = temp;
					parent->children = previous;
					temp = previous;
				}
				else{
					temp = (trieNode*)malloc(sizeof(trieNode));
					if(temp == NULL)
						return WORKER_MEM_ERROR;

					temp->letter = word[i];
					temp->isEndOfWord = False;
					temp->children = NULL;
					temp->next = previous->next;
					previous->next = temp;
				}
			}

			if(i == strlen(word)-1) {
				temp->isEndOfWord = True;
			}
			
			previous = NULL;	
		}
	}

	return WORKER_OK;
}

int initializeTrie(int distr, worker_map** map_ptr, trieNode* node){
		
	char* token;
	int errorCode;

	for(int i = 0; i < distr; i++){
		for(int j = 0; j < map_ptr[0][i].total_files; j++){
			for(int k = 0; k < map_ptr[0][i].dirFiles[j].lines; k++){
				
				/* Retrieving the line */
				char* lineCopy = (char*)malloc((strlen(map_ptr[0][i].dirFiles[j].ptr[k].line_content)+1)*sizeof(char));
				if(lineCopy == NULL)
					return WORKER_MEM_ERROR;
				strcpy(lineCopy,map_ptr[0][i].dirFiles[j].ptr[k].line_content);

				/* Splitting the line into its words */
				token = strtok(lineCopy," \t");
				while(token != NULL){

					/* Insert a word with the newline character
					   at the end of it */
					if(token[strlen(token)-1] == '\n'){
						char* tmp;
						tmp = (char*)malloc(strlen(token)*sizeof(char));
						if(tmp == NULL)
							return WORKER_MEM_ERROR;
						strncpy(tmp,token,strlen(token)-1);
						tmp[strlen(token)-1] = '\0';

						errorCode = insertTrie(node,tmp);
						if(errorCode != WORKER_OK)
							return errorCode;

						free(tmp);
					}

					else{
						errorCode = insertTrie(node,token);
						if(errorCode != WORKER_OK)
							return errorCode;
					}

					token = strtok(NULL," \t");
				}
				free(lineCopy);
			}
		}
	}
	return WORKER_OK;
}

void destroyTrie(trieNode* node){
	if(node == NULL)
		return;

	destroyTrie(node->next);
	destroyTrie(node->children);
	free(node);
}