#define WORKER_OK 0
#define WORKER_ARGS_ERROR -1
#define WORKER_RAISE_ERROR -2
#define WORKER_OPEN_ERROR -3
#define WORKER_MEM_ERROR -4
#define WORKER_DIR_OPEN_ERROR -5
#define WORKER_DIR_CLOSE_ERROR -6
#define WORKER_EXIT -7

typedef struct worker_map{
	int dirID;
	char* dirPath;
}worker_map;

typedef struct line{
	int line_id;
	char* line_content;
}line;

typedef struct file_info{
	char* file_location;
	char* file_name;
	int total_lines;
	line* file_lines;
}file_info;

void printWorkerError(int);
int workerArgs(int);
void freeingMemory(int*, int*, worker_map**, int);
int createWorkerMap(worker_map**, int);
int initializeWorkerMap(worker_map**, int, int, int);
void printWorkerMap(worker_map**, int);
void deleteWorkerMap(worker_map**, int);
int openingPipes(char*, char*, int*, int*);
int getNumberOfFilesInDirs(int, worker_map*);
int getNumberOfLines(FILE*);
int manageLines(int, file_info**, int, worker_map**);
void printLinesStruct(file_info**, int);