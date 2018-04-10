#define OK 0
#define MEM_ERROR -1
#define FEW_ARG -2
#define NOT_ENOUGH_ARG -3
#define MANY_ARG -4
#define FILE_NOT_OPEN -5
#define FORK_ERROR -6
#define PIPE_ERROR -7
#define EXEC_ERROR -8
#define EXIT -9

typedef struct map{
	int dirID;
	char* dirPath;
	int dirLength;
}map;

void printErrorMessage(int);
int CommandLineArg(int);
int getNumberOfLines(FILE*);
int createMap(map**, int);
int initializeMap(FILE*, map*, int);
void printMap(map*, int);
void deleteMap(map**, int);
int createPipePtr(int***, int);
int createPipe(int**);
void deletePipe(int***, int);
int allocatePathname(char**, char**, int);
void createPathname(char**, char*, char**);
