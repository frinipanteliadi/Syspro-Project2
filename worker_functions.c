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

		if(map_ptr[0][i].dirPath[strlen(map_ptr[0][i].dirPath)-1] == '\n'){

			name = (char*)malloc((strlen(map_ptr[0][i].dirPath)+1)*sizeof(char));
			if(name == NULL)
				return WORKER_MEM_ERROR;
			strncpy(name,map_ptr[0][i].dirPath,strlen(map_ptr[0][i].dirPath)-1);
			name[strlen(map_ptr[0][i].dirPath)-1] = '/';
			name[strlen(map_ptr[0][i].dirPath)] = '\0';
		}
		else{
			name = (char*)malloc((strlen(map_ptr[0][i].dirPath)+2)*sizeof(char));
			if(name == NULL)
				return WORKER_MEM_ERROR;

			strcpy(name,map_ptr[0][i].dirPath);
			strcat(name,"/");
			name[strlen(map_ptr[0][i].dirPath)+1] = '\0';
		}

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
			temp_ptr[j].full_path[strlen(name)+strlen(entry[k]->d_name)] = '\0';

			temp_ptr[j].lines = 0;
			temp_ptr[j].ptr = NULL;
		}

		/* Release the memory that scandir() allocated */
		for(int j = 0; j < n; j++)
			free(entry[j]);
		free(entry);
	
		free(name);
	}
	return WORKER_OK;
}

/* Find the total number of lines in the file and parse it in the structure */
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

/* Read all of the lines in the file and parse them in the structure */
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

/**********************/
/*** TRIE FUNCTIONS ***/
/**********************/

/* Creates and initializes the root of the Trie */
int createRoot(trieNode **root){

	*root = (trieNode*)malloc(sizeof(trieNode));
	if(*root == NULL)
		return WORKER_MEM_ERROR;

	(**root).letter = 0;
	(**root).isEndOfWord = False;
	(**root).children = NULL;
	(**root).next = NULL;
	(**root).listPtr = NULL;

	return WORKER_OK;
}

/* Returns the difference between the ASCII values
   of a and b */
int compareKeys(char* a, char* b){
	return((*a)-(*b));
} 

/* Inserts a new value (word) in the Trie */
int insertTrie(trieNode* node, char* word, char* id){
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
			temp->children->listPtr = NULL;

			temp = temp->children;

			if(i == strlen(word)-1) {
				temp->isEndOfWord = True;
				errorCode = addList(&(temp->listPtr), id, word);
				if(errorCode != WORKER_OK)
					return errorCode;
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
				temp->listPtr = NULL;
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
					previous->listPtr = NULL;
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
					temp->listPtr = NULL;
					previous->next = temp;
				}
			}

			if(i == strlen(word)-1) {
				temp->isEndOfWord = True;
				errorCode = addList(&(temp->listPtr), id, word);
				if(errorCode != WORKER_OK)
					return errorCode;
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

						errorCode = insertTrie(node,tmp,map_ptr[0][i].dirFiles[j].ptr[k].id);
						if(errorCode != WORKER_OK)
							return errorCode;

						free(tmp);
					}

					else{
						errorCode = insertTrie(node,token,map_ptr[0][i].dirFiles[j].ptr[k].id);
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

/* Deallocates all memory associated with the Trie */
void destroyTrie(trieNode* node){
	if(node == NULL)
		return;

	destroyTrie(node->next);
	destroyTrie(node->children);
	deletePostingsList(node->listPtr);
	free(node);
}

/* Prints a node of the Trie structure */
void printNode(trieNode* node){
	printf("\n****************************\n");
	printf("* Letter: %c\n",node->letter);
	printf("* isEndOfWord: %d\n",node->isEndOfWord);
	// printf("* Children: %x\n",node->children);
	// printf("* Next: %x\n",node->next);
	printf("****************************\n");
}

/* Prints each node of the Trie from left to right
   and from down to up */
void printTrie(trieNode* node){
	if(node == NULL)
		return;

	printTrie(node->next);
	printTrie(node->children);
	printNode(node);
}

/* Searches for the given word in the Trie */
postingsList* searchTrie(trieNode* node,char* word){
	trieNode* temp = node;
	trieNode* previous = NULL;
	int value;
	for(int i = 0; i < strlen(word); i++){
		
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

		if(temp == NULL)
			return NULL;
		
		if(value < 0)
			return NULL;
		
		if(i == strlen(word)-1){
			if(temp->isEndOfWord == True){
				return temp->listPtr;
			}
			else
				return NULL;
		}

		previous = NULL;
		
	}
	return NULL;
}


/*********************/
/*** POSTINGS LIST ***/ 
/***   FUNCTIONS   ***/
/*********************/

int addList(postingsList** ptr, char* id, char* theWord){

	postingsList* parent;
	postingsListNode* temp;
	postingsListNode* previous;
	int value;

	/* A postings list hasn't been created yet */
	if(*ptr == NULL){
		*ptr = (postingsList*)malloc(sizeof(postingsList));
		if(*ptr == NULL)
			return WORKER_MEM_ERROR;
		
		(*ptr)->lfVector = 1;
		
		(*ptr)->word = (char*)malloc(strlen(theWord)+1);
		if((*ptr)->word == NULL)
			return WORKER_MEM_ERROR;
		strcpy((*ptr)->word,theWord);
		
		/* Creating the head of the postings list */
		(*ptr)->headPtr = (postingsListNode*)malloc(sizeof(postingsListNode));
		if((*ptr)->headPtr == NULL)
			return WORKER_MEM_ERROR;

		(*ptr)->headPtr->lineID = (char*)malloc((strlen(id)+1)*sizeof(char));
		if((*ptr)->headPtr->lineID == NULL)
			return WORKER_MEM_ERROR;
		strcpy((*ptr)->headPtr->lineID,id);
		
		// (*ptr)->headPtr->occurences = 1;
		
		(*ptr)->headPtr->next = NULL;
		
	}
	
	else{
		parent = *ptr;
		temp = parent->headPtr;

		while(temp != NULL){
			value = strcmp(id,temp->lineID);

			if(value == 0)
				break;
			if(value > 0){
				previous = temp;
				temp = temp->next;
				continue;
			}
		}

		/*if(value == 0)
			temp->occurences++;
		else */if(value > 0){
			temp = (postingsListNode*)malloc(sizeof(postingsListNode));
			if(temp == NULL)
				return WORKER_MEM_ERROR;
			temp->lineID = (char*)malloc((strlen(id)+1)*sizeof(char));
			if(temp->lineID == NULL)
				return WORKER_MEM_ERROR;
			strcpy(temp->lineID,id);
			// temp->occurences = 1;
			temp->next = previous->next;
			previous->next = temp;
			parent->lfVector++;
		}
	}

	return WORKER_OK;
}

void deleteList(postingsListNode* ptr){
	if(ptr == NULL)
		return;
	deleteList(ptr->next);
	free(ptr->lineID);
	free(ptr);
}

void deletePostingsList(postingsList* ptr){
	if(ptr == NULL)
		return;

	postingsListNode* temp = ptr->headPtr;
	deleteList(temp);
	free(ptr->word);
	free(ptr);
}

void printPostingsList(postingsList* ptr, char* word){
	if(ptr == NULL){
		printf("The word %s is not in the Trie\n",word);
		return;
	}
	postingsListNode* temp = ptr->headPtr;

	printf("\n******************\n");
	printf("WORD: %s\n",ptr->word);
	printf("lfVector: %d\n",ptr->lfVector);
	while(temp != NULL){
		printf("------------------\n");
		printf("lineID: %s\n",temp->lineID);
		// printf("Occurences: %d\n",temp->occurences);
		// printf("Next: %x\n",temp->next);
		printf("------------------\n");
		temp = temp->next;
	}
	printf("******************\n\n");
}

void printAllLF(trieNode* node){
	if(node == NULL)
		return;
	printAllLF(node->children);
	printAllLF(node->next);
	if(node->isEndOfWord == True)
		printf("%s %d\n",node->listPtr->word,node->listPtr->lfVector);
}

/* Responsible for the reading end of the pipe */
int readPipe(int fd_read, int fd_write, char** args){
	
	/* Fetch the size of the incoming data */	
	char length[1024];
	memset(length,'\0',1024);
	read(fd_read,length,1024);
	length[strlen(length)] = '\0';
	int string_length = atoi(length);

	/* Respond positively */
	write(fd_write,"OK",strlen("OK"));

	/* Allocate memory for the incoming data */
	*args = (char*)malloc((string_length+1)*sizeof(char));
	if(*args == NULL)
		return WORKER_MEM_ERROR;

	memset(*args,'\0',string_length+1);

	/* Get the data */
	read(fd_read,*args,(size_t)string_length*sizeof(char));
	args[0][string_length] = '\0';

	/* Respond positively */
	write(fd_write,"OK",strlen("OK"));
	
	return WORKER_OK;
}

int readDirectories(int fd_read, int fd_write, int distr, worker_map** map_ptr){
	
	int errorCode;
	char length[1024];
	int string_length;

	for(int i = 0; i < distr; i++){

		memset(length,'\0',1024);
		read(fd_read,length,1024);
		length[strlen(length)] = '\0';
		string_length = atoi(length);

		write(fd_write, "OK", strlen("OK"));

		errorCode = initializeWorkerMap(map_ptr,i,fd_read,string_length);
		if(errorCode != WORKER_OK)
			return errorCode;

		write(fd_write,"OK",strlen("OK"));
	}
	return WORKER_OK;
}


/* Returns the number of words in a list of arguments */
int getTotalArguments(char* arguments){
	
	char* arguments_copy;
	arguments_copy = (char*)malloc((strlen(arguments)+1)*sizeof(char));
	if(arguments_copy == NULL)
		return WORKER_MEM_ERROR;
	strcpy(arguments_copy,arguments);

	int total_arguments = 0;
	char* token;
	token = strtok(arguments_copy," \t");
	while(token != NULL){
		total_arguments++;
		token = strtok(NULL," \t");
	}

	free(arguments_copy);
	return total_arguments;
}

int keepArguments(char** wordKeeping, char* arguments){

	char* arguments_copy = (char*)malloc((strlen(arguments)+1)*sizeof(char));
	if(arguments_copy == NULL)
		return WORKER_MEM_ERROR;
	strcpy(arguments_copy,arguments);

	int i = 0;
	char* token;
	token = strtok(arguments_copy," \t");
	while(token != NULL){
		wordKeeping[i] = (char*)malloc((strlen(token)+1)*sizeof(char));
		if(wordKeeping[i] == NULL)
			return WORKER_MEM_ERROR;
		strcpy(wordKeeping[i],token);
		i++;
		token = strtok(NULL," \t");
	}

	free(arguments_copy);
	return WORKER_OK;
}

void getIDs(int* array, char* id){

	int i = 0;
	char* token;
	token = strtok(id,"|");
	while(token != NULL){
		array[i] = atoi(token);
		token = strtok(NULL,"|");
		i++;
	}
}

/* Prints all the infomation associated with the directories 
   and the files in them */
void printStructs(int distr, worker_map** map_ptr){

	for(int i = 0; i < distr; i++){
		printf("\n**************************************\n");
		printf(" -dirID: %s (%d)\n",map_ptr[0][i].dirID, (int)strlen(map_ptr[0][i].dirID));
		printf(" -dirPath: %s (%d)",map_ptr[0][i].dirPath,(int)strlen(map_ptr[0][i].dirPath));
		printf(" -TotalFiles: %d\n",map_ptr[0][i].total_files);

		for(int j = 0; j < map_ptr[0][i].total_files; j++){
			printf("  -fileID: %s (%d)\n",map_ptr[0][i].dirFiles[j].file_id,(int)strlen(map_ptr[0][i].dirFiles[j].file_id));
			printf("  -fileName: %s (%d)\n",map_ptr[0][i].dirFiles[j].file_name,(int)strlen(map_ptr[0][i].dirFiles[j].file_name));
			printf("  -fullPath: %s (%d)\n",map_ptr[0][i].dirFiles[j].full_path,(int)strlen(map_ptr[0][i].dirFiles[j].full_path));
			printf("  -Lines: %d\n",map_ptr[0][i].dirFiles[j].lines);

			for(int k = 0; k < map_ptr[0][i].dirFiles[j].lines; k++){
				printf("   -lineID: %s (%d)\n",map_ptr[0][i].dirFiles[j].ptr[k].id,(int)strlen(map_ptr[0][i].dirFiles[j].ptr[k].id));
				printf("   -lineContent: %s (%d)",map_ptr[0][i].dirFiles[j].ptr[k].line_content,(int)strlen(map_ptr[0][i].dirFiles[j].ptr[k].line_content));
			}
		}

		printf("**************************************\n\n");
	}
}