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
void freeingMemory(int*, int*, worker_map**,int);
int createWorkerMap(worker_map**, int);
int initializeWorkerMap(worker_map**, int, int, int);
void printWorkerMap(worker_map**, int);
void deleteWorkerMap(worker_map**, int);
int openingPipes(char*, char*, int*, int*);