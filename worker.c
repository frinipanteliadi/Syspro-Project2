#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include "worker_functions.h"

int main(int argc, char* argv[]){
	/*printf("\n**WORKER**\n");
	int errorCode;
	if(errorCode = workerArgs(argc) != OK){
		printWorkerError(errorCode);
		return EXIT;
	}

	char* pathname_read = argv[2];		
	char* pathname_write = argv[1];								
	printf("(Child)Reads from: %s\n",argv[2]);
	printf("(Child)Writes to: %s\n",argv[1]);

	int fd = open(argv[2],O_RDONLY);*/
	
	printf("** WORKER **\n");
	int fd = open(argv[1],O_RDONLY);	
	printf("Message:\n");
	char c;
	while(read(fd,&c,1) == 1){												
		putchar(c);
	}
	
	close(fd);
	return OK;
}