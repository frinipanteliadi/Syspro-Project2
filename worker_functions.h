#define WORKER_OK 0
#define WORKER_ARGS_ERROR -1
#define WORKER_RAISE_ERROR -2
#define WORKER_OPEN_ERROR -3
#define WORKER_MEM_ERROR -4
#define WORKER_EXIT -5

typedef struct worker_map{
	int dirID;
	char* dirPath;
}worker_map;

void printWorkerError(int);
int workerArgs(int);
int getTotalLines(char*);
int createWorkerMap(worker_map**, int);
int initializeWorkerMap(worker_map**, char*);
void printWorkerMap(worker_map**, int);
void deleteWorkerMap(worker_map**, int);
void signal_handler(int);