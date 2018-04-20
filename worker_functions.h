#define WORKER_OK 0
#define WORKER_ARGS_ERROR -1
#define WORKER_RAISE_ERROR -2
#define WORKER_OPEN_ERROR -3
#define WORKER_MEM_ERROR -4
#define WORKER_DIR_OPEN_ERROR -5
#define WORKER_DIR_CLOSE_ERROR -6
#define WORKER_EXIT -7

/* One for each line of a certain file */
typedef struct line_info{
	int id;
	char* line_content;
}line_info;

/* One for each file of the directory*/
typedef struct file_info{
	char* file_name;
	char* full_path;
	int lines;
	line_info* ptr;
}file_info;

/* One for every directory that was provided */
typedef struct worker_map{
	int dirID;
	char* dirPath;
	int total_files;
	file_info *dirFiles;
}worker_map;


void printWorkerError(int);
int workerArgs(int);

int createWorkerMap(worker_map**, int);
int initializeWorkerMap(worker_map**, int, int, int);
void printWorkerMap(worker_map**, int);
void deleteWorkerMap(worker_map**, int);

int openingPipes(char*, char*, int*, int*);

int getNumberOfLines(FILE*);
int fileInformation(int, worker_map**);

int setLines(int, worker_map**);

int initializeLinePointer(int, worker_map**);

int readLines(int, worker_map**);

int initializeStructs(int, worker_map**);

void printDirectory(int, worker_map**);