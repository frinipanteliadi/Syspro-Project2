#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
	int errorCode = CommandLineArg(argc);								// Checking the number of arguments that were provided
	if(errorCode != OK){
		printErrorMessage(errorCode);
		return EXIT;
	}

	int numWorkers = 10;												// Default value for the number of Worker processes
	FILE* fp;					

	for(int i = 1; i < argc; i+=2){										// The arguments can be provided in different ways
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

	int total_directories = getNumberOfLines(fp);
	rewind(fp);															// Return to the beggining of the file
	
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

	/***************************/
	/*** DEALLOCATING MEMORY ***/
	/***************************/

	deleteMap(&mapPtr,total_directories);
	fclose(fp);															// Closing the file
}