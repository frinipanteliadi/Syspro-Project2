#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "worker_functions.h"

int main(int argc, char* argv[]){

	int errorCode;

	if(errorCode = workerArgs(argc) != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}

	/****************************/
	/*** GETTING THE NAMES OF ***/ 
	/***      THE PIPES       ***/
	/****************************/

	char* pathname_read = argv[2];		
	int fd_read;
	
	char* pathname_write = argv[1];	
	int fd_write;
	
	errorCode = openingPipes(pathname_read, pathname_write,
		&fd_read,&fd_write);
	if(errorCode != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}

	/*printf("(Child)Reads from: %s\n",argv[2]);
	printf("(Child)Writes to: %s\n",argv[1]);*/
	
	int worker_no = atoi(argv[3]);											// Worker's number
	int distr = atoi(argv[4]);												// Number of paths for the worker

	worker_map *map_ptr;
	errorCode = createWorkerMap(&map_ptr,distr);
	if(errorCode != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}

	// printf("** WORKER(%d) **\n",worker_no);							
	
	/********************************/
	/*** FETCHING THE DIRECTORIES ***/
	/***       VIA PIPES          ***/ 
	/********************************/

	errorCode = readDirectories(fd_read, fd_write, distr, &map_ptr);
	if(errorCode != WORKER_OK){
		printWorkerError(errorCode);
		return WORKER_EXIT;
	}
	
	// printWorkerMap(&map_ptr,distr);

	/****************************/
	/*** RETRIEVING THE FILES ***/
	/***        FROM          ***/
	/***   THE DIRECTORIES    ***/
	/****************************/
	
	errorCode = initializeStructs(distr,&map_ptr);
	if(errorCode != WORKER_OK){
		printf("ERROR!\n");
		return -1;
	}

	// printDirectory(distr, &map_ptr);

	/* Creating the root of the Trie */
	trieNode* root;
	errorCode = createRoot(&root);
	
	errorCode = initializeTrie(distr,&map_ptr,root);
	if(errorCode != WORKER_OK)
		return -1;

	// printTrie(root);

	// printWorkerMap(&map_ptr,distr);
	
	// printAllLF(root);	

	// printStructs(distr,&map_ptr);


	/*********************************/
	/*** USER REQUESTED OPERATIONS ***/
	/*********************************/

	/* Getting the first operation */

	char* input;
	errorCode = readPipe(fd_read,fd_write,&input);
	if(errorCode != WORKER_OK)
		return WORKER_EXIT;
	printf("\nWorker: %s\n",input);
	
	/* Keep answering user requested operations until
	   the user asks to quit the application */
	while(strcmp(input,"/exit") != 0){

		char* operation;
		char* arguments;

		operation = strtok(input," \t");
		arguments = strtok(NULL,"\n");

		if(strcmp(operation,"/search") == 0){
			printf("\nThe worker will perform a %s operation\n",operation);
			printf("The arguments are: %s\n",arguments);
				
			int total = getTotalArguments(arguments);

			/* Store each one of the words in the list of arguments */
			char** wordKeeping;
			wordKeeping = (char**)malloc(total*sizeof(char*));
			if(wordKeeping == NULL)
				return WORKER_MEM_ERROR;
			
			if(keepArguments(wordKeeping,arguments) < 0)
				return WORKER_EXIT;


			/* Finding the total number of results we will be sending */
			int results = 0;
			char* word;
			word = strtok(arguments," \t");
			while(word != NULL){
				
				postingsList* ptr;
				ptr = searchTrie(root,word);
				if(ptr == NULL){
					printf("%s is not in any of the",word);
					printf(" provided files\n");
					word = strtok(NULL," \t");
					continue;
				}

				postingsListNode* temp;
				temp = ptr->headPtr;
				while(temp != NULL){
					results++;
					temp = temp->next;
				}
				word = strtok(NULL," \t");
			}
			printf("\nThe worker will return %d results\n",results);
			
			/* Inform the parent about the number of results it will receive */
			write(fd_write,&results,sizeof(int));

			/* Find out parent's response */
			char response[3];
			read(fd_read,response,strlen("OK"));
			response[2] = '\0';
			if(strcmp(response,"OK") != 0)
				return WORKER_EXIT;

			/* Start sending the results */
			for(int i = 0; i < total; i++){
				// printf("Word: %s\n",wordKeeping[i]);

				postingsList* ptr;
				ptr = searchTrie(root,wordKeeping[i]);
				if(ptr == NULL)
					continue;

				postingsListNode* temp;
				temp = ptr->headPtr;
				while(temp != NULL){

					char* line_id;
					line_id = (char*)malloc((strlen(temp->lineID)+1)*sizeof(char));
					if(line_id == NULL)
						return WORKER_MEM_ERROR;
					strcpy(line_id,temp->lineID);

					int turn = 0;
					int mapID, fileID, lineID;
					char* token;
					token = strtok(line_id,"|");
					while(token != NULL){
						if(turn == 0)
							mapID = atoi(token);
						else if(turn == 1)
							fileID = atoi(token);
						else if(turn == 2){
							lineID = atoi(token);
							break;
						}
						token = strtok(NULL,"|");
						turn++;
					}

					free(line_id);

					// printf("MapID: %d\n",mapID);
					// printf("FileID: %d\n",fileID);
					// printf("LineID: %d\n", lineID);
					printf("\n");

					/* Sending the word we're working on */
					char length[1024];
					memset(length,'\0',1024);
					sprintf(length,"%ld",strlen(wordKeeping[i]));
					write(fd_write,length,1024);

					char parent_response[3];
					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;

					write(fd_write,wordKeeping[i],strlen(wordKeeping[i]));

					memset(parent_response,'\0',3);
					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;




					/* Sending the file's path */
					memset(length,'\0',1024);
					sprintf(length,"%ld",strlen(map_ptr[mapID].dirFiles[fileID].full_path));
					write(fd_write,length,1024);

					memset(parent_response,'\0',3);
					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;

					// printf("Sending: %s\n",map_ptr[mapID].dirFiles[fileID].full_path);
					write(fd_write,map_ptr[mapID].dirFiles[fileID].full_path,strlen(map_ptr[mapID].dirFiles[fileID].full_path));

					memset(parent_response,'\0',3);
					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;




					/* Sending the index of the line */
					write(fd_write,&lineID,sizeof(int));

					memset(parent_response,'\0',3);
					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;



					/* Sending the length of the line */
					memset(length,'\0',1024);
					sprintf(length,"%ld",strlen(map_ptr[mapID].dirFiles[fileID].ptr[lineID].line_content));
					write(fd_write,length,1024);

					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;

					// printf("Sending: %s\n",map_ptr[mapID].dirFiles[fileID].full_path);
					write(fd_write,map_ptr[mapID].dirFiles[fileID].ptr[lineID].line_content,strlen(map_ptr[mapID].dirFiles[fileID].ptr[lineID].line_content));

					memset(parent_response,'\0',3);
					read(fd_read,parent_response,strlen("OK"));
					parent_response[2] = '\0';
					if(strcmp(parent_response,"OK") != 0)
						return WORKER_EXIT;


					temp = temp->next;
				}
			}

			for(int i = 0; i < total; i++)
				free(wordKeeping[i]);
			free(wordKeeping);
		}
		
		free(input);
		errorCode = readPipe(fd_read,fd_write,&input);
		if(errorCode != WORKER_OK)
			return WORKER_EXIT;
		printf("Worker: %s\n",input);
	}

	





	/*******************/
	/*** TERMINATION ***/
	/*******************/


	for(int i = 0; i < distr; i++){
		free(map_ptr[i].dirID);
		free(map_ptr[i].dirPath);

		for(int j = 0; j < map_ptr[i].total_files; j++){
			free(map_ptr[i].dirFiles[j].file_id);
			free(map_ptr[i].dirFiles[j].file_name);
			free(map_ptr[i].dirFiles[j].full_path);
			
			for(int k = 0; k < map_ptr[i].dirFiles[j].lines; k++){
				free(map_ptr[i].dirFiles[j].ptr[k].line_content);
				free(map_ptr[i].dirFiles[j].ptr[k].id);
			}

			free(map_ptr[i].dirFiles[j].ptr);
		}

		free(map_ptr[i].dirFiles);
	}
	
	free(input);
	close(fd_write);
	close(fd_read);
	free(map_ptr);
	destroyTrie(root);
	return WORKER_OK;
}