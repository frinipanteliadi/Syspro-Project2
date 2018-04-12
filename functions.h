#define OK 0
#define MEM_ERROR -1
#define FEW_ARG -2
#define NOT_ENOUGH_ARG -3
#define MANY_ARG -4
#define FILE_NOT_OPEN -5
#define FORK_ERROR -6
#define PIPE_ERROR -7
#define EXEC_ERROR -8
#define OPEN_ERROR -9
#define RAISE_ERROR -10
#define EXIT -11

typedef struct map{
	int dirID;
	char* dirPath;
}map;

typedef struct pipes{
	int pipe_id;
	char* pipename_read;
	char* pipename_write;
}pipes;

void printErrorMessage(int);
int CommandLineArg(int);
int getNumberOfLines(FILE*);
void distributions(int, int, int*);
void setDistributions(int, int, int*);
int createMap(map**, int);
int initializeMap(FILE*, map*, int);
void printMap(map*, int);
void deleteMap(map**, int);
int createPipePtr(int***, int);
int createPipe(int**);
void deletePipe(int***, int);
int allocatePathname(char**, char**, int);
void createPathname(char**, char*, char**);
int allocatePipeArray(pipes**, int);
int initializePipeArray(pipes**,int, char*, char*);
void deletePipeArray(pipes**, int);
