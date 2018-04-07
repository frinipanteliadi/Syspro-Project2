#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	if(argc != 3){
		printf("Invalid arguments\n");
		return -1;
	}

	int pipefd[2];
	pipefd[0] = atoi(argv[1]);
	pipefd[1] = atoi(argv[2]);

	printf("Child Read FD: %d\n",pipefd[0]);
	printf("Child Write FD: %d\n",pipefd[1]);
	printf("I am the child process\n");
	return 0;
}