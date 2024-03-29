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
	int pipe_read_fd;
	int pipe_write_fd;
	char* pipename_read;
	char* pipename_write;
}pipes;

typedef struct result_info{
	char* word;
	char* file_path;
	int line_number;
	char* line_content;
}result_info;

typedef struct result_map{
	result_info* ptr;
	int total_results;
}result_map;


void printErrorMessage(int);
int CommandLineArg(int);
int getNumberOfLines(FILE*);
void distributions(int, int, int*);
void setDistributions(int, int, int*);
void welcomeMessage(void);
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
int addToPipeArray(pipes**, int, char*, char*, int, int);
void printPipeArray(pipes**, int);
void deletePipeArray(pipes**, int);
void closingPipes(pipes**, int);
int writingPipes(char*, int, int);
int readingPipes(char**, int, int);
int getNumberOfArgs(char*);
int storingWords(char**, char*, int*);